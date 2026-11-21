# Smart Bin — 红外感应自动开盖垃圾桶

STM32F103C8T6 @72MHz。红外感应到人 → 舵机开盖 → 保持 3 秒自动关盖；
红外对射检测桶满 → LED 常亮 + 蜂鸣器间歇 + OLED 提示。

全部非阻塞：主循环 10ms 一拍，用 `sys_ms` 做唯一时间基准。

## 硬件接线

所有引脚集中在 `Bin/BoardConfig.h`，换板子只改这一个文件。

| 功能 | 引脚 | 有效电平 | 说明 |
|---|---|---|---|
| LED（满溢指示） | PA0 | 高电平点亮 | `BOARD_LED_ACTIVE = 1` |
| 蜂鸣器 | PA4 | 低电平响 | `BOARD_BEEP_ACTIVE = 0` |
| 手动按键 KEY1 | PA2 | 按下为低 | 内部上拉 |
| 投放口红外 | PB14 | 检测到目标为低 | 四脚模块 DO→PB14，AO 悬空 |
| 满溢对射 | PB13 | 挡光为高 | 接收端 OUT→PB13，发射端只接 5V/GND |
| 舵机 SG90 | PA8 | — | TIM1_CH1，50Hz，**必须 5V 独立供电并与开发板共地** |
| OLED SCL | PB8 | — | 软件 I2C，开漏 |
| OLED SDA | PB9 | — | 软件 I2C，开漏 |
| 串口 USART3 | PB10 / PB11 | — | TX / RX，115200（可选） |

两路红外的有效电平按模块而定。上电看 OLED 第 3 行的 `RAW` 值：
挡住光路时 `RAW` 会翻转，但第 1 行不出【桶已满】，就是 `BOARD_IR_FULL_ACTIVE` 填反了。

## 目录结构

```
Bin/            应用层，按职责分层
  BoardConfig.h   唯一的引脚/极性表
  BinConfig.h     业务参数与功能开关
  BinApp.c        状态机：什么时候开、什么时候关、什么时候报警
  BinSensor.c     2 路红外 + 按键，周期采样 + 计数去抖
  BinUi.c         OLED 界面，只接收 BinUiFrame_t，不持有状态
  BinTelemetry.c  串口上报
  BinFmt.c        极简整数格式化（不用 printf）
Led/ Beep/       PA0 / PA4 单引脚驱动
Servo/           TIM1 PWM 舵机驱动
Oled/            SSD1306 软件 I2C 驱动与字库
Uart/            USART3 单向发送
System/          SysTick 1ms 时基
User/            main.c、中断向量、标准外设库配置
Library/ Start/  ST 标准外设库与启动文件（原样保留）
```

## 编译

Keil MDK 打开 `Smart_bin.uvprojx`，直接 Build。命令行：

```bash
UV4.exe -b Smart_bin.uvprojx -j0 -o build.log
```

`Objects/`、`Listings/`、`*.uvguix.*` 是构建产物与本机界面布局，已在 `.gitignore` 中排除。

## 常用参数（Bin/BinConfig.h）

| 宏 | 默认 | 作用 |
|---|---|---|
| `BIN_SERVO_OPEN_DEG` / `_CLOSED_DEG` | 90 / 0 | 开合角度 |
| `BIN_LID_HOLD_MS` | 3000 | 开盖后保持时间 |
| `BIN_FULL_CONFIRM_MS` | 1000 | 光路连续遮挡多久判定为满 |
| `BIN_FULL_CLEAR_MS` | 2000 | 恢复多久后解除（清空垃圾） |
| `BIN_BEEP_ON_MS` / `_OFF_MS` | 200 / 800 | 报警间歇节奏 |
| `BIN_LID_KEEP_OPEN_WHILE_HAND` | 0 | 1 = 手不离开就一直续期 |
| `BIN_USE_UART` | 0 | 1 = 打开串口上报 |
| `BIN_USE_IWDG` | 1 | 独立看门狗，1 秒不喂狗复位 |

## 串口输出（`BIN_USE_UART = 1` 时）

```
BIN,BOOT,SmartBin v1.0
EVT,OPEN,IR,cnt=3
EVT,CLOSE
EVT,THROW
EVT,FULL / EVT,CLEAR
BIN,lid=0,full=0,open=3,throw=2,up=125s     每 5 秒一条心跳
```

只发不收。串口接收若要做命令控制，应把字节收进环形缓冲交给主循环解析，
不要在中断里解析和回包。

## 上电自检顺序

1. 开机画面约 0.8 秒 → 清屏进入【待机】
2. 手靠近 → 开盖 90°，第 4 行显示 `Close in 3.0s` → 3 秒后关盖
3. 手一直不放 → 关盖后锁定，手离开才允许再开（不反复开合）
4. 按 KEY1 → 强制开盖 3 秒
5. 挡住对射 1 秒 → LED 常亮 + 蜂鸣器间歇 +【桶已满】；移开 2 秒解除
