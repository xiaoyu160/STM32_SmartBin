#include "stm32f10x.h"
#include "BinSensor.h"
#include "BinConfig.h"

/* ------------------------------------------------------------------
 * 去抖器：连续 N 次采样都与上次相同，才更新"稳定值"
 *
 *   cnt 记的是"当前电平已经连续保持了多少次采样"，
 *   一旦采样值变了就把 cnt 清零重来。
 *   这样 40ms 的毛刺不会传到上层，而真实的慢变化也不会被丢掉。
 * ------------------------------------------------------------------ */
typedef struct
{
    uint8_t  last;      /* 上一次原始电平（0/1，已按有效电平归一化） */
    uint16_t cnt;       /* 连续保持次数 */
    uint8_t  stable;    /* 去抖后的稳定电平 */
} Debounce_t;

static Debounce_t s_hand;
static Debounce_t s_full;
static Debounce_t s_key;

static uint8_t s_key_edge;      /* 按下边沿标志，读后清零 */

/* 读引脚并换算成"1 = 有效" */
static uint8_t read_active(GPIO_TypeDef *port, uint16_t pin, uint8_t active_level)
{
    uint8_t level = (GPIO_ReadInputDataBit(port, pin) == Bit_SET) ? 1 : 0;
    return (level == active_level) ? 1 : 0;
}

static void debounce_step(Debounce_t *d, uint8_t value, uint16_t ticks)
{
    if (value == d->last)
    {
        if (d->cnt < 0xFFFF) d->cnt++;
    }
    else
    {
        d->last = value;
        d->cnt  = 0;
    }

    if ((d->cnt >= ticks) && (d->stable != value))
    {
        d->stable = value;

        /* 按键只在"松开 -> 按下"这一刻产生一次事件 */
        if (d == &s_key && value == 1)
        {
            s_key_edge = 1;
        }
    }
}

void BinSensor_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* 两路红外：模块是开漏/推挽输出，MCU 侧用上拉输入最稳 */
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin   = BIN_IR_HAND_PIN;
    GPIO_Init(BIN_IR_HAND_PORT, &gpio);

    gpio.GPIO_Pin   = BIN_IR_FULL_PIN;
    GPIO_Init(BIN_IR_FULL_PORT, &gpio);

    /* 手动按键：板载按键一端接 GND，内部上拉，按下读到 0 */
    gpio.GPIO_Pin   = BIN_KEY_PIN;
    GPIO_Init(BIN_KEY_PORT, &gpio);

    /* 先采一次初值，避免上电瞬间误判 */
    s_hand.last = s_hand.stable = read_active(BIN_IR_HAND_PORT, BIN_IR_HAND_PIN, BIN_IR_HAND_ACTIVE);
    s_full.last = s_full.stable = read_active(BIN_IR_FULL_PORT, BIN_IR_FULL_PIN, BIN_IR_FULL_ACTIVE);
    s_key.last  = s_key.stable  = read_active(BIN_KEY_PORT,     BIN_KEY_PIN,     BIN_KEY_ACTIVE);

    s_hand.cnt = s_full.cnt = s_key.cnt = BIN_IR_DEBOUNCE_TICKS;
    s_key_edge = 0;
}

void BinSensor_Update(void)
{
    debounce_step(&s_hand, read_active(BIN_IR_HAND_PORT, BIN_IR_HAND_PIN, BIN_IR_HAND_ACTIVE),
                  BIN_IR_DEBOUNCE_TICKS);

    debounce_step(&s_full, read_active(BIN_IR_FULL_PORT, BIN_IR_FULL_PIN, BIN_IR_FULL_ACTIVE),
                  BIN_IR_DEBOUNCE_TICKS);

    debounce_step(&s_key,  read_active(BIN_KEY_PORT,     BIN_KEY_PIN,     BIN_KEY_ACTIVE),
                  BIN_KEY_DEBOUNCE_TICKS);
}

uint8_t BinSensor_Hand(void) { return s_hand.stable; }
uint8_t BinSensor_Full(void) { return s_full.stable; }
uint8_t BinSensor_Key(void)  { return s_key.stable;  }

uint8_t BinSensor_KeyPressed(void)
{
    uint8_t e = s_key_edge;
    s_key_edge = 0;
    return e;
}

uint8_t BinSensor_RawHand(void)
{
    return (GPIO_ReadInputDataBit(BIN_IR_HAND_PORT, BIN_IR_HAND_PIN) == Bit_SET) ? 1 : 0;
}

uint8_t BinSensor_RawFull(void)
{
    return (GPIO_ReadInputDataBit(BIN_IR_FULL_PORT, BIN_IR_FULL_PIN) == Bit_SET) ? 1 : 0;
}
