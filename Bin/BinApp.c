#include "stm32f10x.h"
#include "BinApp.h"
#include "BinConfig.h"
#include "BinSensor.h"
#include "Servo.h"
#include "OLED.h"
#include "beep.h"
#include "UART.h"
#include "systick.h"
#include "Delay.h"

/* ==================================================================
 * 智能垃圾桶 —— 应用层状态机
 *
 *   上电
 *    └─> IDLE(盖子 0°) ──红外感应/手动按键──> OPEN(盖子 90°)
 *          ^                                     │
 *          └────────── 保持 3 秒 ────────────────┘
 *
 *   "满"是叠加在盖子状态上的一个标志位，不影响开合：
 *   对射光路连续遮挡 1 秒 -> 满：LED 常亮 + 蜂鸣器间歇响 + OLED 提示
 *   光路恢复 2 秒          -> 解除满
 *
 * 全部非阻塞：主循环 10ms 一拍，用 sys_ms 做时间基准，
 * 不用 Delay_ms 死等，所以报警、倒计时、串口上报互不影响。
 * ================================================================== */

typedef enum
{
    LID_CLOSED = 0,
    LID_OPEN   = 1
} LidState_t;

/* ---------------- 运行状态 ---------------------------------------- */
static LidState_t s_lid;            /* 盖子状态 */
static uint32_t   s_lid_tick;       /* 本次开盖的起始时刻 */
static uint32_t   s_open_cnt;       /* 开盖次数 */
static uint8_t    s_wait_release;   /* 关盖时手还在感应区：等手离开才允许再开，
                                       否则会出现"关上立刻又弹开"的反复开合 */

static uint8_t    s_full;           /* 1 = 桶已满 */
static uint32_t   s_full_tick;      /* 光路开始被遮挡的时刻 */
static uint32_t   s_empty_tick;     /* 光路开始恢复的时刻 */

static uint8_t    s_beam_prev;      /* 上一次的对射状态（判边沿用） */
static uint32_t   s_beam_tick;      /* 光路被挡的起始时刻（算投放时长） */

static uint8_t    s_beep_seg;       /* 0 = 静音段，1 = 鸣叫段 */
static uint32_t   s_beep_tick;

static uint32_t   s_throw_cnt;      /* 投放次数（对射被短暂遮挡） */

static uint32_t   s_tel_tick;       /* 上一次心跳上报 */
static uint32_t   s_ui_tick;        /* 上一次刷屏 */

static void BinApp_LedWrite(uint8_t on);   /* 前置声明 */

/* ==================================================================
 * 小工具
 * ================================================================== */

static char *put_str(char *p, const char *s)
{
    while (*s) *p++ = *s++;
    return p;
}

static char *put_u32(char *p, uint32_t v, uint8_t width, char pad)
{
    char    tmp[12];
    uint8_t n = 0;
    uint8_t i;

    if (v == 0)
    {
        tmp[n++] = '0';
    }
    while (v && n < 12)
    {
        tmp[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (n < width && n < 12)
    {
        tmp[n++] = pad;
    }
    for (i = n; i > 0; i--) *p++ = tmp[i - 1];
    return p;
}

/* 打印 1 位小数的秒数：ms -> "12.3" */
static char *put_sec(char *p, uint32_t ms)
{
    p = put_u32(p, ms / 1000u, 1, ' ');
    *p++ = '.';
    p = put_u32(p, (ms % 1000u) / 100u, 1, '0');
    return p;
}

/* ==================================================================
 * 输出设备
 * ================================================================== */

static void led_init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* 只初始化 PA0。注意不要用 LED_INIT()：
       它会把 PA0~PA3 全配成输出，而 PA2 在这里是手动按键输入。 */
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin   = BIN_LED_PIN;
    GPIO_Init(BIN_LED_PORT, &gpio);

    BinApp_LedWrite(0);
}

static void BinApp_LedWrite(uint8_t on)
{
    BitAction level = on ? (BitAction)BIN_LED_ACTIVE : (BitAction)(!BIN_LED_ACTIVE);
    GPIO_WriteBit(BIN_LED_PORT, BIN_LED_PIN, level);
}

/* ==================================================================
 * 盖子动作
 * ================================================================== */

static void lid_close(void)
{
    Servo_SetAngle(BIN_SERVO_CLOSED_DEG);
    s_lid = LID_CLOSED;

    /* 到点关盖时手还停在感应区：先锁住，等手离开再解锁，
       避免"关上->立刻又检测到人->再开"的无限开合 */
    if (BinSensor_Hand()) s_wait_release = 1;
}

/* why: "IR" / "KEY"，只用于串口日志 */
static void lid_open(const char *why)
{
    if (s_lid == LID_CLOSED)
    {
        Servo_SetAngle(BIN_SERVO_OPEN_DEG);
        s_open_cnt++;

#if BIN_USE_UART
        {
            char  buf[32];
            char *p = buf;

            p = put_str(p, "EVT,OPEN,");
            p = put_str(p, why);
            p = put_str(p, ",cnt=");
            p = put_u32(p, s_open_cnt, 1, '0');
            p = put_str(p, "\r\n");
            *p = 0;
            USART3_SendString(buf);
        }
#endif
        (void)why;
    }

    s_lid      = LID_OPEN;
    s_lid_tick = sys_ms;            /* 重新计时 3 秒 */
}

/* ==================================================================
 * 串口上报
 * ================================================================== */

static void telemetry(void)
{
#if BIN_USE_UART
    char buf[64];
    char *p = buf;

    p = put_str(p, "BIN,lid=");
    p = put_u32(p, (uint32_t)s_lid, 1, '0');
    p = put_str(p, ",full=");
    p = put_u32(p, s_full, 1, '0');
    p = put_str(p, ",open=");
    p = put_u32(p, s_open_cnt, 1, '0');
    p = put_str(p, ",throw=");
    p = put_u32(p, s_throw_cnt, 1, '0');
    p = put_str(p, ",up=");
    p = put_u32(p, sys_ms / 1000u, 1, '0');
    p = put_str(p, "s\r\n");
    *p = 0;
    USART3_SendString(buf);
#endif
}

static void evt(const char *name)
{
#if BIN_USE_UART
    char buf[48];
    char *p = buf;

    p = put_str(p, "EVT,");
    p = put_str(p, name);
    p = put_str(p, "\r\n");
    *p = 0;
    USART3_SendString(buf);
#endif
}

/* ==================================================================
 * OLED 界面
 * ================================================================== */

/* 固定写满 16 个字符，余下补空格 —— 避免上一次的长内容留下"残影" */
static void ui_line(uint8_t line, const char *s)
{
    char    buf[17];
    uint8_t i;

    for (i = 0; i < 16; i++) buf[i] = ' ';
    buf[16] = '\0';

    for (i = 0; s[i] && i < 16; i++) buf[i] = s[i];

    OLED_ShowString(line, 1, buf);
}

static void ui_blank(char *buf)
{
    uint8_t i;
    for (i = 0; i < 16; i++) buf[i] = ' ';
    buf[16] = '\0';
}

/* 从第 col 列（0 基）开始放一段 ASCII，超出的截掉 */
static void ui_put_at(char *buf, uint8_t col, const char *s)
{
    while (*s && col < 16) buf[col++] = *s++;
}

/* 界面
 *   第 1 行：【待机】/【开盖】/【桶已满】+ 英文补充
 *   第 2 行：投放次数 / 开盖次数
 *   第 3 行：两路红外原始状态（接线自检就看这行）
 *   第 4 行：关盖倒计时 / 请清空 / 按键提示 */
static void ui_render(void)
{
    char    buf[17];
    uint32_t left;

    /* ---------- 第 1 行：状态（汉字） + 英文补充 ---------- */
    ui_blank(buf);

    if (s_full)
    {
        /* 三个 16x16 汉字占前 48 像素，英文从第 56 像素开始 */
        ui_put_at(buf, 7, "ALARM");
        OLED_ShowString(1, 1, buf);

        OLED_ShowChinese(1, 1, BIN_HZ_TONG);    /* 桶 */
        OLED_ShowChinese(1, 2, BIN_HZ_YI);      /* 已 */
        OLED_ShowChinese(1, 3, BIN_HZ_MAN);     /* 满 */
    }
    else if (s_lid == LID_OPEN)
    {
        ui_put_at(buf, 6, "LID UP");
        OLED_ShowString(1, 1, buf);

        OLED_ShowChinese(1, 1, BIN_HZ_KAI);     /* 开 */
        OLED_ShowChinese(1, 2, BIN_HZ_GAI);     /* 盖 */
    }
    else
    {
        ui_put_at(buf, 6, "READY");
        OLED_ShowString(1, 1, buf);

        OLED_ShowChinese(1, 1, BIN_HZ_DAI);     /* 待 */
        OLED_ShowChinese(1, 2, BIN_HZ_JI);      /* 机 */
    }

    /* ---------- 第 2 行：投放 / 开盖次数 ---------- */
    {
        char *p = buf;
        p = put_str(p, "Cnt:");
        p = put_u32(p, s_throw_cnt % 1000u, 3, '0');
        p = put_str(p, " Open:");
        p = put_u32(p, s_open_cnt % 100u, 2, '0');
        *p = 0;
        ui_line(2, buf);
    }

    /* ---------- 第 3 行：红外感应 + 对射引脚原始电平 ----------
     * RAW 就是 PB13 的电平，没经过极性判断。
     * 挡住光路看这个数字：它必须跟着变，否则是接线/供电问题；
     * 它变了但第 1 行不变【桶已满】，就是 BIN_IR_FULL_ACTIVE 填反了。 */
    {
        char *p = buf;
        p = put_str(p, "IR:");
        p = put_u32(p, BinSensor_Hand(), 1, '0');
        p = put_str(p, " RAW:");
        p = put_u32(p, BinSensor_RawFull(), 1, '0');
        *p = 0;
        ui_line(3, buf);
    }

    /* ---------- 第 4 行：提示 / 关盖倒计时 ---------- */
    if (s_lid == LID_OPEN)
    {
        char *p = buf;
        left = (sys_ms - s_lid_tick >= BIN_LID_HOLD_MS)
             ? 0u : (BIN_LID_HOLD_MS - (sys_ms - s_lid_tick));
        p = put_str(p, "Close in ");
        p = put_sec(p, left);
        p = put_str(p, "s");
        *p = 0;
        ui_line(4, buf);
    }
    else if (s_full)
    {
        ui_line(4, "EMPTY THE BIN!");
    }
    else
    {
        ui_line(4, "KEY:OPEN LID");
    }
}

/* ==================================================================
 * 看门狗
 * ================================================================== */

static void wdt_init(void)
{
#if BIN_USE_IWDG
    /* LSI 约 40kHz，64 分频 -> 625Hz，重装 625 -> 约 1 秒超时 */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_64);
    IWDG_SetReload(625);
    IWDG_ReloadCounter();
    IWDG_Enable();
#endif
}

/* ==================================================================
 * 10ms 周期任务：采样 + 状态机 + 输出
 * ================================================================== */

static void tick_10ms(void)
{
    uint32_t now = sys_ms;
    uint8_t  beam;

    BinSensor_Update();
    beam = BinSensor_Full();

    {
        uint8_t hand = BinSensor_Hand();

        if (!hand) s_wait_release = 0;              /* 手已离开，解锁 */

        /* ---------- 1. 开盖触发 ---------- */
        if (BinSensor_KeyPressed())                 /* 手动按键：强制开盖 3 秒 */
        {
            lid_open("KEY");
        }
        else if (hand && s_lid == LID_CLOSED && !s_wait_release)
        {
            lid_open("IR");                         /* 红外感应到人：开盖 */
        }

#if BIN_LID_KEEP_OPEN_WHILE_HAND
        /* 手还没离开就续期，避免盖子压手；离开后再等满 3 秒关 */
        if (s_lid == LID_OPEN && hand &&
            (now - s_lid_tick) >= BIN_LID_RETRIGGER_MS)
        {
            s_lid_tick = now;
        }
#endif

        /* ---------- 2. 到点关盖 ---------- */
        if (s_lid == LID_OPEN && (now - s_lid_tick) >= BIN_LID_HOLD_MS)
        {
            lid_close();
            evt("CLOSE");
        }
    }

    /* ---------- 3. 满溢判定（带确认时间，防抖 + 防误报） ---------- */
    if (beam)
    {
        s_empty_tick = now;
        if (!s_full && (now - s_full_tick) >= BIN_FULL_CONFIRM_MS)
        {
            s_full      = 1;
            s_beep_seg  = 1;            /* 立刻进入"响"的那一段，不要先干等 800ms */
            s_beep_tick = now;
            evt("FULL");
        }
    }
    else
    {
        s_full_tick = now;
        if (s_full && (now - s_empty_tick) >= BIN_FULL_CLEAR_MS)
        {
            s_full = 0;
            evt("CLEAR");
        }
    }

    /* ---------- 4. 投放计数：对射被短暂遮挡一次 = 投了一件 ---------- */
    if (beam && !s_beam_prev)
    {
        s_beam_tick = now;
    }
    else if (!beam && s_beam_prev)
    {
        if (!s_full && (now - s_beam_tick) >= 50u && (now - s_beam_tick) < BIN_FULL_CONFIRM_MS)
        {
            s_throw_cnt++;
            evt("THROW");
        }
    }
    s_beam_prev = beam;

    /* ---------- 5. 报警输出（每拍都写，状态永远确定） ---------- */
    if (s_full)
    {
        uint32_t seg_len = s_beep_seg ? BIN_BEEP_ON_MS : BIN_BEEP_OFF_MS;

        BinApp_LedWrite(1);                     /* LED 常亮 */

        if ((now - s_beep_tick) >= seg_len)
        {
            s_beep_tick = now;
            s_beep_seg  = !s_beep_seg;
        }

        if (s_beep_seg) Beep_On(); else Beep_Off();
    }
    else
    {
        BinApp_LedWrite(0);
        Beep_Off();
        s_beep_seg  = 0;
        s_beep_tick = now;
    }

    /* ---------- 6. 心跳上报 ---------- */
    if ((now - s_tel_tick) >= BIN_TELEMETRY_MS)
    {
        s_tel_tick = now;
        telemetry();
    }
}

/* ==================================================================
 * 对外接口
 * ================================================================== */

void BinApp_Init(void)
{
    BinSensor_Init();
    Servo_Init();                       /* PA8, 50Hz, 先输出 0.5ms(0°) */
    Beep_Init();                        /* PA4 推挽，默认不响 */
    led_init();                         /* PA0 */
    OLED_Init();
    OLED_Clear();

    OLED_ShowString(1, 1, "  SMART BIN");
    OLED_ShowString(2, 1, "  STM32F103C8");
    OLED_ShowString(3, 1, "  Servo+IR+OLED");
    Delay_ms(800);                      /* 此时 SysTick 还没接管，用 Delay 安全 */

#if BIN_SERVO_SELFTEST
    /* 上电自检：开一下再关，确认舵机通路正常（BinConfig.h 里可关掉） */
    Servo_SetAngle(BIN_SERVO_OPEN_DEG);
    Delay_ms(600);
    Servo_SetAngle(BIN_SERVO_CLOSED_DEG);
    Delay_ms(600);
#endif

    SysTick_Init();                     /* 之后一律用 sys_ms，别再调 Delay_ms */
    OLED_Clear();

#if BIN_USE_UART
    USART3_Init(BIN_UART_BAUD);
    USART3_SendString("BIN,BOOT,SmartBin v1.0\r\n");
#endif

    /* 关盖 + 状态初始化 */
    Servo_SetAngle(BIN_SERVO_CLOSED_DEG);
    s_lid        = LID_CLOSED;
    s_lid_tick   = sys_ms;
    s_open_cnt   = 0;
    s_wait_release = 0;
    s_full       = 0;
    s_full_tick  = sys_ms - BIN_FULL_CONFIRM_MS;    /* 上电时如果已经满，1 秒后立刻报警 */
    s_empty_tick = sys_ms;
    s_beam_prev  = 0;
    s_beam_tick  = sys_ms;
    s_beep_seg   = 0;
    s_beep_tick  = sys_ms;
    s_throw_cnt  = 0;
    s_tel_tick   = sys_ms;
    s_ui_tick    = sys_ms - BIN_UI_REFRESH_MS;

    wdt_init();
}

void BinApp_Run(void)
{
    uint32_t next = sys_ms;

    while (1)
    {
        /* ---- 10ms 软定时器：不用 Delay，延时不会拖慢其它任务 ---- */
        if ((int32_t)(sys_ms - next) >= 0)
        {
            next += BIN_LOOP_MS;
            tick_10ms();
        }

        /* ---- 刷屏节流 ---- */
        if ((sys_ms - s_ui_tick) >= BIN_UI_REFRESH_MS)
        {
            s_ui_tick = sys_ms;
            ui_render();
        }

        /* ---- 喂狗：主循环还转着就说明没死机 ---- */
#if BIN_USE_IWDG
        IWDG_ReloadCounter();
#endif

#if BIN_USE_IDLE_SLEEP
        __WFI();        /* 等下一个 SysTick 中断再继续，省电 */
#endif
    }
}
