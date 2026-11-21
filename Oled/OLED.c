#include "stm32f10x.h"
#include "OLED.h"
#include "OLED_Font.h"
#include "BoardConfig.h"

/* ------------------------------------------------------------------
 * 0.96 寸 128x64 OLED，SSD1306，软件 I2C
 *
 *   引脚：PB8 = SCL，PB9 = SDA，开漏输出，两个脚都到 BoardConfig.h 取
 *   坐标：Line 1~4（上到下），Column 1~16（左到右）
 *
 * 软件 I2C 没有时钟延展，靠指令间隔自然拉开时序，72MHz 下足够慢。
 * ------------------------------------------------------------------ */

#define OLED_ADDR               0x78        /* 0x3C << 1 */
#define OLED_CTRL_CMD           0x00
#define OLED_CTRL_DATA          0x40

#define OLED_W_SCL(x)   GPIO_WriteBit(BOARD_OLED_PORT, BOARD_OLED_SCL_PIN, (BitAction)(x))
#define OLED_W_SDA(x)   GPIO_WriteBit(BOARD_OLED_PORT, BOARD_OLED_SDA_PIN, (BitAction)(x))

/* ==================================================================
 * 软件 I2C
 * ================================================================== */

static void i2c_init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Mode  = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin   = BOARD_OLED_SCL_PIN;
    GPIO_Init(BOARD_OLED_PORT, &gpio);

    gpio.GPIO_Pin   = BOARD_OLED_SDA_PIN;
    GPIO_Init(BOARD_OLED_PORT, &gpio);

    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

static void i2c_start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SDA(0);      /* SCL 高电平期间 SDA 下降 = 起始 */
    OLED_W_SCL(0);
}

static void i2c_stop(void)
{
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_W_SDA(1);      /* SCL 高电平期间 SDA 上升 = 停止 */
}

/* 高位在前，第 9 个时钟只占时间、不读应答（SSD1306 写操作必应答） */
static void i2c_send_byte(uint8_t byte)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        OLED_W_SDA(!!(byte & (0x80 >> i)));
        OLED_W_SCL(1);
        OLED_W_SCL(0);
    }

    OLED_W_SCL(1);
    OLED_W_SCL(0);
}

static void write_command(uint8_t cmd)
{
    i2c_start();
    i2c_send_byte(OLED_ADDR);
    i2c_send_byte(OLED_CTRL_CMD);
    i2c_send_byte(cmd);
    i2c_stop();
}

static void write_data(uint8_t data)
{
    i2c_start();
    i2c_send_byte(OLED_ADDR);
    i2c_send_byte(OLED_CTRL_DATA);
    i2c_send_byte(data);
    i2c_stop();
}

/* Y：页地址 0~7（每页 8 像素高）；X：列地址 0~127 */
static void set_cursor(uint8_t Y, uint8_t X)
{
    write_command(0xB0 | Y);
    write_command(0x10 | ((X & 0xF0) >> 4));
    write_command(0x00 | (X & 0x0F));
}

/* ==================================================================
 * 对外接口
 * ================================================================== */

void OLED_Clear(void)
{
    uint8_t page, col;

    for (page = 0; page < 8; page++)
    {
        set_cursor(page, 0);
        for (col = 0; col < 128; col++)
        {
            write_data(0x00);
        }
    }
}

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;

    /* 一个 8x16 字符占两页，上下各写 8 字节 */
    set_cursor((Line - 1) * 2, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        write_data(OLED_F8x16[Char - ' '][i]);
    }

    set_cursor((Line - 1) * 2 + 1, (Column - 1) * 8);
    for (i = 0; i < 8; i++)
    {
        write_data(OLED_F8x16[Char - ' '][i + 8]);
    }
}

void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String)
{
    uint8_t i;

    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/* 一个 16x16 汉字占两页、左右各 16 字节 */
void OLED_ShowChinese(uint8_t Line, uint8_t Column, uint8_t Index)
{
    uint8_t i;

    set_cursor((Line - 1) * 2, (Column - 1) * 16);
    for (i = 0; i < 16; i++)
    {
        write_data(OLED_Chinese[Index][i]);
    }

    set_cursor((Line - 1) * 2 + 1, (Column - 1) * 16);
    for (i = 0; i < 16; i++)
    {
        write_data(OLED_Chinese[Index][i + 16]);
    }
}

void OLED_Init(void)
{
    uint32_t i, j;

    /* 上电延时，等 OLED 内部电荷泵和稳压稳定；此时还没有时基，只能空转 */
    for (i = 0; i < 1000; i++)
    {
        for (j = 0; j < 1000; j++);
    }

    i2c_init();

    write_command(0xAE);        /* 关闭显示 */
    write_command(0xD5);        /* 时钟分频 / 振荡频率 */
    write_command(0x80);
    write_command(0xA8);        /* 多路复用率 = 64 */
    write_command(0x3F);
    write_command(0xD3);        /* 显示偏移 = 0 */
    write_command(0x00);
    write_command(0x40);        /* 显示起始行 = 0 */
    write_command(0xA1);        /* 左右方向：正常 */
    write_command(0xC8);        /* 上下方向：正常 */
    write_command(0xDA);        /* COM 引脚硬件配置 */
    write_command(0x12);
    write_command(0x81);        /* 对比度 */
    write_command(0xCF);
    write_command(0xD9);        /* 预充电周期 */
    write_command(0xF1);
    write_command(0xDB);        /* VCOMH 电平 */
    write_command(0x30);
    write_command(0xA4);        /* 按显存内容显示 */
    write_command(0xA6);        /* 正常显示（非反白） */
    write_command(0x8D);        /* 电荷泵 */
    write_command(0x14);
    write_command(0xAF);        /* 开启显示 */

    OLED_Clear();
}
