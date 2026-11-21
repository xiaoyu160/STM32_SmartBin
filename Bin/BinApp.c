#include "stm32f10x.h"
#include "BinApp.h"
#include "BinConfig.h"
#include "BinSensor.h"
#include "BinUi.h"
#include "BinTelemetry.h"
#include "Servo.h"
#include "LED.h"
#include "beep.h"
#include "UART.h"
#include "systick.h"

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
 * 全部非阻塞：主循环 10ms 一拍，用 sys_ms 做时间基准。
 * 界面和串口上报各自在 BinUi.c / BinTelemetry.c 里，
 * 这个文件只管"什么时候开、什么时候关、什么时候报警"。
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
static uint32_t   s_last_clear_tick;/* 光路最近一次"通畅"的时刻 */
static uint32_t   s_last_block_tick;/* 光路最近一次"被挡"的时刻 */

static uint8_t    s_beam_prev;      /* 上一次的对射状态（判边沿用） */
static uint32_t   s_throw_tick;     /* 本次遮挡的起始时刻（算投放时长） */

static uint8_t    s_beep_seg;       /* 0 = 静音段，1 = 鸣叫段 */
static uint32_t   s_beep_tick;

static uint32_t   s_throw_cnt;      /* 投放次数（对射被短暂遮挡） */

static uint32_t   s_tel_tick;       /* 上一次心跳上报 */
static uint32_t   s_ui_tick;        /* 上一次刷屏 */

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
        BinTelemetry_LidOpen(why, s_open_cnt);
    }

    s_lid      = LID_OPEN;
    s_lid_tick = sys_ms;            /* 重新计时 3 秒 */
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
            BinTelemetry_Event("CLOSE");
        }
    }

    /* ---------- 3. 满溢判定（带确认时间，防抖 + 防误报） ----------
     * 遮挡持续 1 秒 -> 判定满；恢复持续 2 秒 -> 解除。
     * 两个"最近时刻"变量分别在遮挡/恢复时刷新，用减法算持续时间。 */
    if (beam)
    {
        s_last_block_tick = now;
        if (!s_full && (now - s_last_clear_tick) >= BIN_FULL_CONFIRM_MS)
        {
            s_full      = 1;
            s_beep_seg  = 1;            /* 立刻进入"响"的那一段，不要先干等 800ms */
            s_beep_tick = now;
            BinTelemetry_Event("FULL");
        }
    }
    else
    {
        s_last_clear_tick = now;
        if (s_full && (now - s_last_block_tick) >= BIN_FULL_CLEAR_MS)
        {
            s_full = 0;
            BinTelemetry_Event("CLEAR");
        }
    }

    /* ---------- 4. 投放计数：对射被短暂遮挡一次 = 投了一件 ---------- */
    if (beam && !s_beam_prev)
    {
        s_throw_tick = now;
    }
    else if (!beam && s_beam_prev)
    {
        if (!s_full && (now - s_throw_tick) >= BIN_THROW_MIN_MS &&
            (now - s_throw_tick) < BIN_FULL_CONFIRM_MS)
        {
            s_throw_cnt++;
            BinTelemetry_Event("THROW");
        }
    }
    s_beam_prev = beam;

    /* ---------- 5. 报警输出（每拍都写，状态永远确定） ---------- */
    if (s_full)
    {
        uint32_t seg_len = s_beep_seg ? BIN_BEEP_ON_MS : BIN_BEEP_OFF_MS;

        LED_Set(1);                             /* LED 常亮 */

        if ((now - s_beep_tick) >= seg_len)
        {
            s_beep_tick = now;
            s_beep_seg  = !s_beep_seg;
        }

        if (s_beep_seg) Beep_On(); else Beep_Off();
    }
    else
    {
        LED_Set(0);
        Beep_Off();
        s_beep_seg  = 0;
        s_beep_tick = now;
    }

    /* ---------- 6. 心跳上报 ---------- */
    if ((now - s_tel_tick) >= BIN_TELEMETRY_MS)
    {
        s_tel_tick = now;
        BinTelemetry_Heartbeat((uint8_t)s_lid, s_full, s_open_cnt, s_throw_cnt);
    }
}

/* ==================================================================
 * 界面刷新（100ms 一次，交给 BinUi 去画）
 * ================================================================== */

static void ui_refresh(void)
{
    BinUiFrame_t f;
    uint32_t     elapsed = sys_ms - s_lid_tick;

    f.lid_open  = (s_lid == LID_OPEN) ? 1 : 0;
    f.full      = s_full;
    f.hand      = BinSensor_Hand();
    f.raw_full  = BinSensor_RawFull();
    f.open_cnt  = s_open_cnt;
    f.throw_cnt = s_throw_cnt;

    f.close_left_ms = (f.lid_open && elapsed < BIN_LID_HOLD_MS)
                    ? (BIN_LID_HOLD_MS - elapsed) : 0u;

    BinUi_Render(&f);
}

/* ==================================================================
 * 对外接口
 * ================================================================== */

void BinApp_Init(void)
{
    BinSensor_Init();
    Servo_Init();                       /* PA8, 50Hz, 先输出 0.5ms(0°) */
    Beep_Init();                        /* PA4 推挽，默认不响 */
    LED_Init();                         /* PA0，只动这一根脚 */
    SysTick_Init();                     /* 先建立 1ms 时基，后面一律用 sys_ms */

    BinUi_Splash();                     /* 开机画面，内部等待 800ms */

#if BIN_SERVO_SELFTEST
    /* 上电自检：开一下再关，确认舵机通路正常（BinConfig.h 里可关掉） */
    Servo_SetAngle(BIN_SERVO_OPEN_DEG);
    SysTick_DelayMs(600);
    Servo_SetAngle(BIN_SERVO_CLOSED_DEG);
    SysTick_DelayMs(600);
    BinUi_Clear();
#endif

#if BIN_USE_UART
    USART3_Init(BIN_UART_BAUD);
    BinTelemetry_Boot();
#endif

    /* 关盖 + 状态初始化 */
    Servo_SetAngle(BIN_SERVO_CLOSED_DEG);
    s_lid             = LID_CLOSED;
    s_lid_tick        = sys_ms;
    s_open_cnt        = 0;
    s_wait_release    = 0;
    s_full            = 0;
    s_last_clear_tick = sys_ms - BIN_FULL_CONFIRM_MS;  /* 上电时如果已经满，1 秒后立刻报警 */
    s_last_block_tick = sys_ms;
    s_beam_prev       = 0;
    s_throw_tick      = sys_ms;
    s_beep_seg        = 0;
    s_beep_tick       = sys_ms;
    s_throw_cnt       = 0;
    s_tel_tick        = sys_ms;
    s_ui_tick         = sys_ms - BIN_UI_REFRESH_MS;

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
            ui_refresh();
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
