#include "stm32f10x.h"
#include "BinSensor.h"
#include "BoardConfig.h"
#include "BinConfig.h"

/* ------------------------------------------------------------------
 * 去抖器：连续 N 次采样都与上次相同，才更新"稳定值"
 *
 *   cnt 记的是"当前电平已经连续保持了多少次采样"，
 *   一旦采样值变了就把 cnt 清零重来。
 *   这样 40ms 的毛刺不会传到上层，而真实的慢变化也不会被丢掉。
 *
 *   edge 只在稳定值由 0 变 1 的那一次置位，由调用者读走并清零，
 *   这样"按键按下"这种一次性事件不会被重复消费。
 * ------------------------------------------------------------------ */
typedef struct
{
    uint8_t  last;      /* 上一次原始电平（0/1，已按有效电平归一化） */
    uint16_t cnt;       /* 连续保持次数 */
    uint8_t  stable;    /* 去抖后的稳定电平 */
    uint8_t  edge;      /* 上升沿标志，读后清零 */
} Debounce_t;

static Debounce_t s_hand;
static Debounce_t s_full;
static Debounce_t s_key;

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
        if (value) d->edge = 1;     /* 只在"变成有效"的那一刻记一次事件 */
    }
}

void BinSensor_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* 两路红外 + 按键：模块是开漏/推挽输出，MCU 侧用上拉输入最稳 */
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin   = BOARD_IR_HAND_PIN;
    GPIO_Init(BOARD_IR_HAND_PORT, &gpio);

    gpio.GPIO_Pin   = BOARD_IR_FULL_PIN;
    GPIO_Init(BOARD_IR_FULL_PORT, &gpio);

    gpio.GPIO_Pin   = BOARD_KEY_PIN;
    GPIO_Init(BOARD_KEY_PORT, &gpio);

    /* 先采一次初值，避免上电瞬间误判 */
    s_hand.last = s_hand.stable = read_active(BOARD_IR_HAND_PORT, BOARD_IR_HAND_PIN, BOARD_IR_HAND_ACTIVE);
    s_full.last = s_full.stable = read_active(BOARD_IR_FULL_PORT, BOARD_IR_FULL_PIN, BOARD_IR_FULL_ACTIVE);
    s_key.last  = s_key.stable  = read_active(BOARD_KEY_PORT,     BOARD_KEY_PIN,     BOARD_KEY_ACTIVE);

    s_hand.cnt = s_full.cnt = s_key.cnt = BIN_IR_DEBOUNCE_TICKS;
    s_hand.edge = s_full.edge = s_key.edge = 0;
}

void BinSensor_Update(void)
{
    debounce_step(&s_hand, read_active(BOARD_IR_HAND_PORT, BOARD_IR_HAND_PIN, BOARD_IR_HAND_ACTIVE),
                  BIN_IR_DEBOUNCE_TICKS);

    debounce_step(&s_full, read_active(BOARD_IR_FULL_PORT, BOARD_IR_FULL_PIN, BOARD_IR_FULL_ACTIVE),
                  BIN_IR_DEBOUNCE_TICKS);

    debounce_step(&s_key,  read_active(BOARD_KEY_PORT,     BOARD_KEY_PIN,     BOARD_KEY_ACTIVE),
                  BIN_KEY_DEBOUNCE_TICKS);
}

uint8_t BinSensor_Hand(void) { return s_hand.stable; }
uint8_t BinSensor_Full(void) { return s_full.stable; }

uint8_t BinSensor_KeyPressed(void)
{
    uint8_t e = s_key.edge;
    s_key.edge = 0;
    return e;
}

uint8_t BinSensor_RawFull(void)
{
    return (GPIO_ReadInputDataBit(BOARD_IR_FULL_PORT, BOARD_IR_FULL_PIN) == Bit_SET) ? 1 : 0;
}
