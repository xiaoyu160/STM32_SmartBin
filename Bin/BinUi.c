#include "stm32f10x.h"
#include "BinUi.h"
#include "BinFmt.h"
#include "BinConfig.h"
#include "OLED.h"
#include "systick.h"

/* 一行 16 个字符，先全部填空格再往里写：
 * 固定写满 16 列，才不会留下上一次的长内容"残影"。 */
static void ui_fill(char *buf)
{
    uint8_t i;
    for (i = 0; i < 16; i++) buf[i] = ' ';
    buf[16] = '\0';
}

/* 从第 col 列（0 基）开始放一段 ASCII，超出右边界的部分丢弃 */
static void ui_put(char *buf, uint8_t col, const char *s)
{
    while (*s && col < 16) buf[col++] = *s++;
}

/* 整行显示：自动补满 16 列 */
static void ui_line(uint8_t line, const char *s)
{
    char buf[17];
    ui_fill(buf);
    ui_put(buf, 0, s);
    OLED_ShowString(line, 1, buf);
}

void BinUi_Clear(void)
{
    OLED_Clear();
}

void BinUi_Splash(void)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "  SMART BIN");
    OLED_ShowString(2, 1, "  STM32F103C8");
    OLED_ShowString(3, 1, "  Servo+IR+OLED");

    SysTick_DelayMs(BIN_BOOT_SPLASH_MS);
    OLED_Clear();
}

void BinUi_Render(const BinUiFrame_t *f)
{
    char buf[17];

    /* ---------- 第 1 行：状态（汉字） + 英文补充 ---------- */
    ui_fill(buf);
    if (f->full)
    {
        ui_put(buf, 7, "ALARM");
        OLED_ShowString(1, 1, buf);
        OLED_ShowChinese(1, 1, BIN_HZ_TONG);    /* 桶 */
        OLED_ShowChinese(1, 2, BIN_HZ_YI);      /* 已 */
        OLED_ShowChinese(1, 3, BIN_HZ_MAN);     /* 满 */
    }
    else if (f->lid_open)
    {
        ui_put(buf, 6, "LID UP");
        OLED_ShowString(1, 1, buf);
        OLED_ShowChinese(1, 1, BIN_HZ_KAI);     /* 开 */
        OLED_ShowChinese(1, 2, BIN_HZ_GAI);     /* 盖 */
    }
    else
    {
        ui_put(buf, 6, "READY");
        OLED_ShowString(1, 1, buf);
        OLED_ShowChinese(1, 1, BIN_HZ_DAI);     /* 待 */
        OLED_ShowChinese(1, 2, BIN_HZ_JI);      /* 机 */
    }

    /* ---------- 第 2 行：投放 / 开盖次数 ---------- */
    {
        char *p;
        ui_fill(buf);
        p = BinFmt_Str(buf, "Cnt:");
        p = BinFmt_U32(p, f->throw_cnt % 1000u, 3, '0');
        p = BinFmt_Str(p, " Open:");
        p = BinFmt_U32(p, f->open_cnt % 100u, 2, '0');
        (void)p;                    /* buf 已补满 16 列，不需要再补 '\0' */
        OLED_ShowString(2, 1, buf);
    }

    /* ---------- 第 3 行：红外感应 + 对射引脚原始电平 ----------
     * RAW 就是 PB13 的电平，没经过极性判断。
     * 挡住光路看这个数字：它必须跟着变，否则是接线/供电问题；
     * 它变了但第 1 行不变【桶已满】，就是 BOARD_IR_FULL_ACTIVE 填反了。 */
    {
        char *p;
        ui_fill(buf);
        p = BinFmt_Str(buf, "IR:");
        p = BinFmt_U32(p, f->hand, 1, '0');
        p = BinFmt_Str(p, " RAW:");
        p = BinFmt_U32(p, f->raw_full, 1, '0');
        (void)p;
        OLED_ShowString(3, 1, buf);
    }

    /* ---------- 第 4 行：提示 / 关盖倒计时 ---------- */
    if (f->lid_open)
    {
        char buf4[17];
        char *p;
        ui_fill(buf4);
        p = BinFmt_Str(buf4, "Close in ");
        p = BinFmt_Sec(p, f->close_left_ms);
        p = BinFmt_Str(p, "s");
        (void)p;
        OLED_ShowString(4, 1, buf4);
    }
    else if (f->full)
    {
        ui_line(4, "EMPTY THE BIN!");
    }
    else
    {
        ui_line(4, "KEY:OPEN LID");
    }
}
