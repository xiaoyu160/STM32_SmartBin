#ifndef __BIN_CONFIG_H
#define __BIN_CONFIG_H

#include "stm32f10x.h"

/* ==================================================================
 * 智能垃圾桶 —— 全局配置（引脚 / 时序 / 功能开关）
 *
 * 所有"想改一下试试"的东西都集中在这一个文件里，
 * 改完重新编译即可，不用去翻各个驱动。
 *
 * MCU : STM32F103C8T6  72MHz
 * 舵机: SG90  PA8 -> TIM1_CH1 (50Hz PWM)
 * ================================================================== */

/* ---------------- 引脚分配 ---------------------------------------- */
/* 舵机信号固定在 PA8 / TIM1_CH1（见 Servo.h，改引脚要连定时器通道一起改） */

/* 红外感应（投放口，检测手/物体靠近）
 *
 * 四脚模块（常见丝印：VCC GND DO AO，LM393 + 反射式探头）：
 *   VCC -> 5V（模块支持 3.3~5V）
 *   GND -> GND
 *   DO  -> PB14        <-- 只用这一根
 *   AO  -> 悬空不接（模拟量输出，本工程不用；它的电压可到 VCC，
 *                     而 PA1 等 ADC 脚不耐 5V，接之前务必确认）
 *
 * 若你的模块"检测到目标"时 DO 输出高电平（看模块上的输出指示灯
 * 和 OLED 第 3 行 IR: 的值），把下面的 0 改成 1 即可，不用改硬件。
 * 三脚模块（E18-D80NK 等）同理，棕VCC / 蓝GND / 黑OUT。 */
#define BIN_IR_HAND_PORT        GPIOB
#define BIN_IR_HAND_PIN         GPIO_Pin_14
#define BIN_IR_HAND_ACTIVE      0       /* 0 = 检测到目标输出低；1 = 输出高 */

/* 红外对射（桶口横向对射，光路被垃圾挡住 = 满）
 *
 * 注意：这是"发射端 + 接收端"两个器件，不是上面那种 4 脚反射模块。
 * 只有接收端有 OUT/DO，接 PB13；发射端只有正负两根线，接 5V 和 GND。
 *
 * 如果满溢也用 4 脚反射模块（装在桶口朝下测堆料高度）：
 *   DO -> PB13，AO 悬空，电位器调到"空桶不触发、堆到位才触发"，
 *   但黑色垃圾袋反射弱、阳光直射会误判，可靠性不如对射。
 *
 * 有效电平怎么定：看 OLED 第 3 行的 RAW 值。
 * 用纸板把光路挡住，记下此刻 RAW 是 0 还是 1，
 *   挡光时 RAW=0  ->  这里填 0
 *   挡光时 RAW=1  ->  这里填 1（本工程现在的模块就是这种）
 * 填对了，挡光时第 1 行才会变成【桶已满】。 */
#define BIN_IR_FULL_PORT        GPIOB
#define BIN_IR_FULL_PIN         GPIO_Pin_13
#define BIN_IR_FULL_ACTIVE      1       /* 1 = 光路被挡输出高；0 = 输出低 */

/* 手动按键（板载 KEY1，内部上拉，按下为低） */
#define BIN_KEY_PORT            GPIOA
#define BIN_KEY_PIN             GPIO_Pin_2
#define BIN_KEY_ACTIVE          0

/* 满溢指示 LED（板载 LED0）
 * 电平极性按实测定：本板是"高电平点亮"，所以填 1。
 * 如果以后换板子发现亮灭又反了，把 1 改成 0 即可。
 * （原工程 LED.c 的注释写的是低电平点亮，但实测这块板相反，
 *   以实测为准，不要照注释改。） */
#define BIN_LED_PORT            GPIOA
#define BIN_LED_PIN             GPIO_Pin_0
#define BIN_LED_ACTIVE          1

/* 蜂鸣器（板载，低电平响） */
#define BIN_BEEP_PORT           GPIOA
#define BIN_BEEP_PIN            GPIO_Pin_4
#define BIN_BEEP_ACTIVE         0

/* OLED: PB8=SCL  PB9=SDA（见 OLED.c，软件 I2C）
 * 串口: PB10=TX  PB11=RX（USART3，见 UART.c） */

/* ---------------- 动作参数 ---------------------------------------- */
#define BIN_SERVO_CLOSED_DEG    0       /* 关盖角度 */
#define BIN_SERVO_OPEN_DEG      90      /* 开盖角度 */
/* 开盖后保持时间：从开盖那一刻算起，满 3 秒就关，不管手还在不在。
 * 手一直停在感应区时，盖子关上后会"锁定"，必须等手离开才允许再开，
 * 所以不会出现反复开合。 */
#define BIN_LID_HOLD_MS         3000u   /* 开盖后保持时间 = 3 秒 */

/* 防夹手模式：
 *   0 = 严格 3 秒（默认，符合"延时 3 秒自动关盖"的要求）
 *   1 = 手不离开就一直续期，离开后再等 3 秒才关（栏杆式垃圾桶的手感） */
#define BIN_LID_KEEP_OPEN_WHILE_HAND 0
#define BIN_LID_RETRIGGER_MS    1000u   /* 仅上面为 1 时生效：续期的最小间隔 */

/* ---------------- 时序 / 滤波参数 --------------------------------- */
#define BIN_LOOP_MS             10u     /* 主循环调度周期 */
#define BIN_IR_DEBOUNCE_MS      40u     /* 红外电平去抖时间 */
#define BIN_KEY_DEBOUNCE_MS     20u     /* 按键去抖时间 */
#define BIN_FULL_CONFIRM_MS     1000u   /* 光路连续遮挡多久判定为"满" */
#define BIN_FULL_CLEAR_MS       2000u   /* 光路恢复多久后解除"满"（清空垃圾） */
#define BIN_BEEP_ON_MS          200u    /* 报警：响 200ms */
#define BIN_BEEP_OFF_MS         800u    /* 报警：停 800ms（间歇） */
#define BIN_UI_REFRESH_MS       100u    /* OLED 最小刷新间隔 */

/* 去抖所需的采样次数（由上面的时间换算，别手改） */
#define BIN_IR_DEBOUNCE_TICKS   (BIN_IR_DEBOUNCE_MS  / BIN_LOOP_MS)
#define BIN_KEY_DEBOUNCE_TICKS  (BIN_KEY_DEBOUNCE_MS / BIN_LOOP_MS)

/* ---------------- 功能开关 ---------------------------------------- */
#define BIN_USE_UART            0       /* 串口上报（PB10/PB11, 115200），不用就 0 */
#define BIN_USE_IWDG            1       /* 独立看门狗，1 秒不喂狗就复位 */
#define BIN_USE_IDLE_SLEEP      0       /* 空闲进 Sleep（细节见文档，默认关） */
#define BIN_UART_BAUD           115200u
#define BIN_TELEMETRY_MS        5000u   /* 心跳上报周期 */

/* 上电舵机自检：设为 1 时，开机画面结束后舵机会 0°->90°->0° 摆一次，
 * 用来确认"舵机 + PWM + 供电"这条通路是好的。
 * 现象是盖子开一下又关上；正式使用请设回 0。 */
#define BIN_SERVO_SELFTEST      0

/* ---------------- 控制方式 ----------------------------------------
 * 本版本只用"全自动"一种方式，没有开关可切：
 *   红外感应到人 -> 舵机 90° 开盖 -> 保持 3 秒自动关盖
 *   按下 KEY1    -> 强制开盖 3 秒后自动关盖
 *   对射被挡 1 秒 -> 满溢报警（LED 常亮 + 蜂鸣器间歇 + OLED【桶已满】）
 *
 * 旋转编码器（PA6/PA7）已从控制逻辑中移除，这两个脚现在是空闲的，
 * 可以留着以后做别的用途。Servo.c 里的 Servo_Demo() 仍然保留，
 * 想单独做"旋钮调角度"的实验时不改主程序也能用。
 * ------------------------------------------------------------------ */

/* ---------------- OLED 汉字索引（OLED_Font.h 中的顺序） ------------ */
#define BIN_HZ_DAI              4       /* 待 */
#define BIN_HZ_JI               5       /* 机 */
#define BIN_HZ_KAI              6       /* 开 */
#define BIN_HZ_GAI              7       /* 盖 */
#define BIN_HZ_TONG             8       /* 桶 */
#define BIN_HZ_YI               9       /* 已 */
#define BIN_HZ_MAN              10      /* 满 */

#endif
