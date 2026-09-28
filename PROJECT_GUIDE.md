# CA51M550 马达振动控制器 - 工程指南

## 📖 目录

- [1. 项目概述](#1-项目概述)
- [2. 硬件规格](#2-硬件规格)
- [3. 系统架构](#3-系统架构)
- [4. 文件结构说明](#4-文件结构说明)
- [5. 快速开始](#5-快速开始)
- [6. 编译与烧录](#6-编译与烧录)
- [7. 功能详解](#7-功能详解)
  - [7.1 按键控制](#71-按键控制)
  - [7.2 LED指示灯](#72-led指示灯)
  - [7.3 马达振动模式](#73-马达振动模式)
  - [7.4 电源管理](#74-电源管理)
- [8. 配置参数说明](#8-配置参数说明)
- [9. API参考手册](#9-api参考手册)
- [10. 移植指南](#10-移植指南)
- [11. 故障排除](#11-故障排除)
- [12. 版本历史](#12-版本历史)

---

## 1. 项目概述

### 1.1 项目简介

本项目是一个基于 **CA51M550 MCU** 的**低功耗马达振动控制系统**，具有以下特点：

- ✅ **10种振动模式**：从简单到复杂，满足不同应用场景
- ✅ **超低功耗设计**：STOP模式下电流 < 7μA
- ✅ **智能电源管理**：5分钟无操作自动关机 + 充电器插拔唤醒
- ✅ **模块化架构**：高度解耦，易于维护和移植
- ✅ **工业级代码质量**：符合C89规范，完整文档注释

### 1.2 应用场景

| 场景 | 推荐模式 | 说明 |
|------|---------|------|
| 📱 手机振动器 | Pattern_1/2/3 | 来电/消息提醒 |
| 🎮 游戏手柄 | Random | 游戏反馈 |
| ⏰ 闹钟提醒 | Pattern_3 + Ramp_Up | 渐强唤醒 |
| 🔔 门铃/警报 | Continuous / Pattern_2 | 持续/紧急提示 |
| 💆 按摩设备 | Interval_1S/2S | 节奏按摩 |
| 🎵 音乐节拍同步 | Pattern_1/2/3 | 循环节奏 |

### 1.3 技术特性

```
技术栈：
├── MCU: CA51M550 (8051内核, 12MHz)
├── 语言: C89 (ANSI C)
├── 架构: 状态机 + 时间片轮询 (非阻塞)
├── 设计模式: HAL + 策略 + 观察者
└── 功耗: 运行 < 5mA / STOP < 7μA
```

---

## 2. 硬件规格

### 2.1 GPIO引脚分配

| 引脚 | 功能 | 方向 | 电平特性 | 说明 |
|------|------|------|----------|------|
| **P0.0** | 按键输入 | 输入 | 内部上拉，按下=LOW | 长按开/关机，短按切换模式 |
| **P0.1** | 充电状态 | 输入 | 内部上拉，充电中=LOW | LOW=充电中，HIGH=充满 |
| **P0.2** | LED指示灯 | 输出 | 高电平点亮 | 开机常亮，充电闪烁 |
| **P0.3** | 马达控制 | 输出 | 高电平开启 | 驱动MOSFET控制马达 |
| **P0.5** | 充电器检测 | 输入 | 无上下拉 | HIGH=插入，LOW=拔出 |

### 2.2 硬件连接图

```
                    ┌─────────────────┐
                    │                 │
    按键 ──────────┤ P0.0 (内部上拉)  │
                    │                 │
    充电IC状态 ─────┤ P0.1 (内部上拉)  │     ┌──────────┐
                    │                 │     │          │
    LED ───────────┤ P0.2 ────[LED]──┼─────┤  GND     │
                    │                 │     │          │
    MOSFET Gate ───┤ P0.3 ────[MOS]──┼─────┤  马达    │
                    │                 │     │          │
    充电器分压 ─────┤ P0.5 (浮空)      │     └──────────┘
                    │                 │         │
                    │   CA51M550       │        GND
                    │                 │
    电池 ──────────┤ VCC             │
                    │                 │
    GND ───────────┤ GND             │
                    │                 │
                    └─────────────────┘
```

### 2.3 外围电路要求

#### 按键电路（P0.0）
```
P0.0 ────┬────[按键]──── GND
         │
        [10K] 上拉电阻（MCU内部）
```

#### LED电路（P0.2）
```
VCC ────[330Ω]────┬──── LED(+) 
                  │
P0.2 ─────────────┴──── LED(-)
```

#### 马达驱动电路（P0.3）
```
VCC(Battery) ────┬──── 马达(+)
                 │
               [MOSFET] N沟道
                 │
P0.3 ───────────┤ Gate
                 │
GND ─────────────┤ Source
                 │
                 └──── 马达(-)
```

#### 充电器检测电路（P0.5）
```
Charger_VCC(5V) ──[R1:10K]──┬─── P0.5
                               │
                              [R2:10K]
                               │
GND ──────────────────────────┘

分压比 = R2/(R1+R2) = 0.5
当充电器插入时：P0.5 ≈ 2.5V (HIGH)
当充电器拔出时：P0.5 = 0V (LOW)
```

---

## 3. 系统架构

### 3.1 软件架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        main.c                                │
│                   （主程序协调器）                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ button  │  │   led   │  │  motor  │  │  power  │        │
│  │  .h/.c  │  │  .h/.c  │  │  .h/.c  │  │  .h/.c  │        │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘        │
│       │            │            │            │              │
│  ┌────┴────────────┴────────────┴────────────┴────┐        │
│  │                  gpio.h / gpio.c                │        │
│  │              （硬件抽象层 HAL）                  │        │
│  └─────────────────────┬──────────────────────────┘        │
│                        │                                   │
│  ┌─────────────────────┴──────────────────────────┐        │
│  │                config.h                         │        │
│  │           （集中配置管理）                       │        │
│  └─────────────────────┬──────────────────────────┘        │
│                        │                                   │
│  ┌─────────────────────┴──────────────────────────┐        │
│  │              ca51m550.h                         │        │
│  │          （MCU寄存器定义）                      │        │
│  └────────────────────────────────────────────────┘        │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 数据流图

```
用户操作
   │
   ▼
┌─────────┐    事件     ┌─────────────┐
│  按键    │ ─────────> │   主程序     │
│  模块   │            │  (main.c)   │
└─────────┘            └──────┬──────┘
                              │
          ┌───────────────────┼───────────────────┐
          ▼                   ▼                   ▼
   ┌──────────┐        ┌──────────┐        ┌──────────┐
   │  LED模块  │        │  马达模块  │        │ 电源模块  │
   │ (显示反馈)│        │ (执行动作)│        │ (功耗管理)│
   └──────────┘        └──────────┘        └──────────┘
          │                   │                   │
          ▼                   ▼                   ▼
   ┌──────────────────────────────────────────────────┐
   │                  GPIO HAL 层                     │
   │            (gpio.h / gpio.c)                     │
   └──────────────────────────────────────────────────┘
          │                   │                   │
          ▼                   ▼                   ▼
   ┌──────────────────────────────────────────────────┐
   │                 CA51M550 硬件                     │
   └──────────────────────────────────────────────────┘
```

### 3.3 状态机总览

#### 系统主状态机
```
                    ┌──────────────┐
                    │              │
         ┌─────────>│   POWER_OFF  │<──────────┐
         │          │  (STOP模式)  │           │
         │          │              │           │
         │          └──────┬───────┘           │
         │                 │                   │
         │    长按按键/     │ 插入充电器/       │
         │    插入充电器    │ 5分钟超时/        │
         │                 │ 拔出充电器        │
         │                 ▼                   │
         │          ┌──────┴───────┐           │
         │          │              │           │
         └──────────│   POWER_ON   │───────────┘
                    │  (运行模式)  │
                    │              │
                    └──────────────┘
```

#### 马达模式状态机
```
IDLE ←→ CONTINUOUS ←→ INTERVAL_1S ←→ INTERVAL_2S
   ↑                                    ↓
   └──── PATTERN_1 ←→ PATTERN_2 ←→ PATTERN_3
                          ↑
              RAMP_UP ←→ RAMP_DOWN
                          ↑
                        RANDOM
```

---

## 4. 文件结构说明

### 4.1 完整文件清单

```
project/
│
├── 📄 PROJECT_GUIDE.md      ← 本工程指南（你正在阅读的）
├── 📄 ca51m550.h            ← MCU寄存器定义（厂商提供）
├── 📄 config.h              ← 集中配置管理（⭐核心配置文件）
│
├── 📁 hardware_abstraction/
│   ├── gpio.h               ← GPIO抽象层接口
│   └── gpio.c               ← GPIO抽象层实现
│
├── 📁 modules/
│   ├── button.h / button.c  ← 按键输入模块
│   ├── led.h / led.c        ← LED指示灯模块
│   ├── motor.h / motor.c    ← 马达控制模块（10种模式）
│   └── power.h / power.c    ← 电源管理模块
│
└── 📄 main.c                ← 主程序入口
```

### 4.2 各文件职责

#### **config.h** - 配置中心
```c
/* 这是整个项目的"控制面板"，修改这里即可调整系统行为 */

/* 系统时钟 */
#define F_CPU  12000000UL  /* 修改晶振频率 */

/* 引脚分配 */
#define BUTTON_PIN  GPIO_PIN_0  /* 修改按键引脚 */
#define LED_PIN     GPIO_PIN_2  /* 修改LED引脚 */
#define MOTOR_PIN   GPIO_PIN_3  /* 修改马达引脚 */

/* 时间参数 */
#define BUTTON_LONG_PRESS_MS  1500  /* 修改长按阈值 */
#define AUTO_POWEROFF_TIME_MS 300000  /* 修改自动关机时间 */

/* ... 更多配置 ... */
```

#### **gpio.h/c** - 硬件抽象层
```c
/* 所有硬件操作都通过这个接口，移植时只需修改此文件 */
void GPIO_Init(pin, mode);      /* 初始化引脚 */
void GPIO_WritePin(pin, level); /* 写电平 */
GPIO_Level_t GPIO_ReadPin(pin); /* 读电平 */
```

#### **button.h/c** - 按键模块
```c
/* 处理按键输入，输出标准化事件 */
Button_Event_t Button_GetEvent();  /* 获取：短按/长按/无事件 */
void Button_Poll();                /* 1ms调用一次 */
```

#### **led.h/c** - LED模块
```c
/* 控制LED的各种显示模式 */
LED_SetState(LED_STATE_ON);       /* 常亮 */
LED_SetState(LED_STATE_BLINK_CHARGING);  /* 充电闪烁 */
LED_StartBlink500ms();            /* 闪烁500ms后恢复 */
```

#### **motor.h/c** - 马达模块
```c
/* 核心功能模块，10种振动模式 */
Motor_SetMode(MOTOR_MODE_PATTERN_1);  /* 设置模式 */
Motor_Poll();                         /* 1ms调用一次 */
```

#### **power.h/c** - 电源管理
```c
/* 低功耗和充电管理 */
Power_EnterStopMode();            /* 进入STOP模式 (<7μA) */
Power_GetChargerState();          /* 获取充电状态 */
```

#### **main.c** - 主程序
```c
/* 协调各模块工作，处理业务逻辑 */
main() {
    System_Init();
    while(1) {
        /* OFF → STOP模式等待唤醒 */
        /* ON  → 轮询各模块 */
    }
}
```

---

## 5. 快速开始

### 5.1 环境准备

#### 必需工具
- ✅ **Keil uVision** (推荐 v5.x 或更高版本)
- ✅ **CA51M550 设备支持包** (DKP)
- ✅ **C51编译器** (Keil内置)
- ✅ **USB转串口下载器** (如CH340, CP2102)

#### 可选工具
- 🔧 **逻辑分析仪** (调试时序用)
- 🔧 **示波器** (观察PWM波形)
- 🔧 **万用表** (测量电压电流)
- 🔧 **电流表** (测量STOP模式功耗)

### 5.2 第一个工程（5分钟上手）

#### 步骤1：创建Keil工程
```
1. 打开 Keil uVision → Project → New uVision Project
2. 保存到 project/ 目录，命名为 MotorController.uvprojx
3. 选择设备：CA51M550 (如果没有，需安装DKP)
```

#### 步骤2：添加源文件
```
在 Project 窗口中创建以下分组：

📁 Source Files
   ├─ main.c
   ├─ button.c
   ├─ led.c
   ├─ motor.c
   ├─ power.c
   └─ gpio.c

📁 Header Files (可选，用于快速导航)
   ├─ ca51m550.h
   ├─ config.h
   ├─ gpio.h
   ├─ button.h
   ├─ led.h
   ├─ motor.h
   └─ power.h
```

#### 步骤3：配置工程选项
```
Options for Target → C51 选项卡:
├─ Define: __CA51M550__
├─ Optimization: Level 8 (平衡优化)
└─ Warnings: All Warnings

Options for Target → Output 选项卡:
└─ ☑ Create HEX File

Options for Target → Debug 选项卡:
└─ Use: 你的仿真器/下载器型号
```

#### 步骤4：设置包含路径
```
Options for Target → C51 → Include Paths:
添加: ..\project  (或你的实际路径)
```

#### 步骤5：编译下载
```
1. 按 F7 (或 Build 按钮) 编译
2. 连接下载器到板子
3. 按 F8 (或 Download 按钮) 烧录
4. 按复位按钮运行！
```

---

## 6. 编译与烧录

### 6.1 编译命令行（高级用户）

如果使用命令行编译：

```bash
# 设置环境变量
set KEIL_PATH=C:\Keil_v5\C51\BIN

# 编译所有C文件
%KEIL_PATH%\C51.EXE *.c DEBUG OBJECTEXTEND CODE

# 链接生成HEX
%KEIL_PATH%\LX51.EXE *.obj TO MotorController HEXFILE (MotorController.hex)

# 查看内存占用
%KEIL_PATH%\OH51.EXE MotorController
```

### 6.2 烧录方法

#### 方法1：ISP串口烧录（推荐）
```
1. 板子进入ISP模式（按住BOOT键再上电）
2. 连接USB转串口
3. 使用厂商提供的ISP工具
4. 选择COM口和波特率（通常115200）
5. 打开HEX文件，点击"下载"
6. 等待完成，复位运行
```

#### 方法2：SWD/JTAG调试器
```
1. 连接调试器（如ULINK, ST-Link等）
2. Keil中设置Debug选项
3. F5启动调试会话
4. F8下载程序
5. F5全速运行
```

#### 方法3：量产烧录
```
对于批量生产，建议：
1. 使用专用烧录座/夹具
2. 编写批处理脚本自动化
3. 使用多路烧录器并行烧录
4. 烧录后自动校验
```

### 6.3 编译常见错误及解决

| 错误信息 | 原因 | 解决方法 |
|---------|------|----------|
| `C141: syntax error near ';'` | C99语法 | 改为C89语法 |
| `C202: 'xxx' undefined identifier` | 未包含头文件 | 添加 #include |
| `C247: non-address/-constant initializer` | 静态数组初始化错误 | 检查初始化值 |
| `L110: cannot find file` | 路径错误 | 检查Include Paths |
| `C129: missing ';' before 'xxx'` | 缺少分号 | 检查上一行 |

---

## 7. 功能详解

### 7.1 按键控制

#### 工作原理
```
物理按键 → P0.0电平变化 → 20ms消抖 → 状态判断 → 事件生成
                                                    │
                                        ┌───────────┴───────────┐
                                        ▼                       ▼
                                  按下时间 < 1.5s           按下时间 ≥ 1.5s
                                        │                       │
                                        ▼                       ▼
                                 BUTTON_EVENT_SHORT_PRESS  BUTTON_EVENT_LONG_PRESS
                                 (短按：切换模式)          (长按：开/关机)
```

#### 时序图
```
时间轴:  0ms    20ms   1500ms   1500ms+dt
         │      │       │        │
P0.0:  HIGH ──┘──── LOW ───────┘──── HIGH
         │      │       │        │
状态:   释放   消抖中  计时中    释放
         │      │       │        │
事件:   NONE   NONE    NONE   SHORT/LONG
```

#### 使用示例
```c
/* 在主循环中轮询 */
void main(void) {
    Button_Init();
    
    while(1) {
        Button_Poll();  /* 必须每1ms调用一次 */
        
        switch(Button_GetEvent()) {
            case BUTTON_EVENT_SHORT_PRESS:
                /* 处理短按：切换马达模式 */
                break;
                
            case BUTTON_EVENT_LONG_PRESS:
                /* 处理长按：开关机 */
                break;
                
            default:
                break;
        }
        
        Button_ClearEvent();  /* 清除已处理的事件 */
        Delay_ms(1);
    }
}
```

#### 配置参数（在config.h中修改）
```c
#define BUTTON_DEBOUNCE_MS    20    /* 消抖时间(ms) */
#define BUTTON_LONG_PRESS_MS  1500  /* 长按阈值(ms) */
```

---

### 7.2 LED指示灯

#### 状态行为表

| 系统状态 | 充电状态 | LED行为 | 说明 |
|---------|---------|---------|------|
| 关机 | - | 熄灭 | 完全关闭 |
| 开机 | 无充电器 | **常亮** | 正常工作 |
| 开机 | 充电中 | **500ms闪烁** | 正在充电 |
| 开机 | 已充满 | **常亮** | 充电完成 |
| 开机 | - | **熄灭500ms** | 短按切换模式的瞬间反馈 |

#### 状态转换图
```
                    ┌────────────┐
                    │   OFF      │
                    └─────┬──────┘
                          │ 开机
                          ▼
                    ┌────────────┐
              ┌─────>│    ON      │<────┐
              │      └─────┬──────┘     │
              │            │            │
              │  充电插拔   │ 短按按键   │
              │            ▼            │
              │    ┌────────────┐       │
              │    │ BLINK_CHARG│       │
              │    │  _ING      │       │
              │    └─────┬──────┘       │
              │          │ 充满/拔出     │
              │          └──────────────┘
              │
              └──── 熄灭500ms后恢复ON
```

#### 使用示例
```c
/* 场景1：开机时点亮LED */
Power_SetState(POWER_STATE_ON);
LED_SetState(LED_STATE_ON);

/* 场景2：检测到充电器插入 */
if (charger_detected) {
    LED_SetState(LED_STATE_BLINK_CHARGING);
}

/* 场景3：短按时的视觉反馈 */
void on_short_press(void) {
    Motor_NextMode();
    LED_StartBlink500ms();  /* 熄灭500ms后自动恢复 */
}

/* 在主循环中刷新LED */
while(1) {
    LED_Poll();  /* 必须1ms调用一次 */
    Delay_ms(1);
}
```

#### 配置参数
```c
#define LED_BLINK_CHARGING_PERIOD   250  /* 闪烁半周期(ms)，500ms完整周期 */
#define LED_BLINK_500MS_DURATION    500  /* 短按熄灭持续时间(ms) */
```

---

### 7.3 马达振动模式

#### 模式概览表

| 模式ID | 名称 | 时序特点 | 适用场景 | 复杂度 |
|--------|------|----------|----------|--------|
| 0 | IDLE | 不振动 | 待机/省电 | ⭐ |
| 1 | CONTINUOUS | 持续振动 | 持续提醒/测试 | ⭐ |
| 2 | INTERVAL_1S | 500ms振+500ms停 | 节奏感提醒 | ⭐⭐ |
| 3 | INTERVAL_2S | 500ms振+1500ms停 | 低频间歇提醒 | ⭐⭐ |
| 4 | PATTERN_1 | 短-短-长停 | 消息通知 | ⭐⭐⭐ |
| 5 | PATTERN_2 | 三连击+长间隔 | 紧急警报 | ⭐⭐⭐ |
| 6 | PATTERN_3 | 长短组合 | 来电/闹钟 | ⭐⭐⭐⭐ |
| 7 | RAMP_UP | 渐强振动 | 唤醒/蓄力 | ⭐⭐⭐ |
| 8 | RAMP_DOWN | 渐弱振动 | 平缓停止 | ⭐⭐⭐ |
| 9 | RANDOM | 随机振动 | 游戏/趣味交互 | ⭐⭐⭐⭐ |

#### 详细时序图

##### **模式0: IDLE（空闲）**
```
时间:  0ms ─────────────────────────────────────→ ∞
马达:  OFF ────────────────────────────────────── OFF
      └───────────────────────────────────────────┘
      持续关闭，零功耗（除了静态电流）
```

##### **模式1: CONTINUOUS（持续）**
```
时间:  0ms ─────────────────────────────────────→ ∞
马达:  ON ─────────────────────────────────────── ON
      └───────────────────────────────────────────┘
      持续开启，最大功耗
```

##### **模式2: INTERVAL_1S（1秒周期）**
```
时间:  0    500  1000  1500  2000  2500  3000 (ms)
马达:  |ON  |OFF |ON   |OFF  |ON   |OFF  |ON
       └───┘    └───┘    └───┘    └───┘
       500ms    500ms    500ms    500ms
       
周期: 1000ms (1秒)
占空比: 50%
```

##### **模式3: INTERVAL_2S（2秒周期）**
```
时间:  0    500  2000  2500  4000  4500 (ms)
马达:  |ON  |OFF |ON   |OFF  |ON   |OFF
       └───┘    └────────┘    └────────┘
       500ms     1500ms        1500ms
       
周期: 2000ms (2秒)
占空比: 25%
```

##### **模式4: PATTERN_1（节奏1：短-短-长停）**
```
时间:  0    200  300  500  600  1100 1200 (ms)
马达:  |ON  |OFF |ON  |OFF |ON   |OFF  |ON...
       └─┘   └─┘  └─┘   └───┘   └─┘
       200ms 100ms 200ms  500ms  (循环)
       
特点: "滴-答----" 的节奏感
适用: 微信消息、邮件通知
```

##### **模式5: PATTERN_2（节奏2：三连击+长停）**
```
时间:  0   100  200  300  400  500  1200 1300 (ms)
马达:  |ON |OFF|ON |OFF|ON |OFF|ON   |OFF |ON...
       └─┘ └─┘ └─┘ └─┘ └─┘   └───┘  └─┘
       100m 100m 100m 100m 100m  700m  (循环)
       
特点: "滴滴滴-------" 紧急感
适用: 警报、电话铃声、紧急通知
```

##### **模式6: PATTERN_3（节奏3：长短组合）**
```
时间:  0   300  500  600  1000 1150 1800 (ms)
马达:  |ON  |OFF|ON |OFF|ON   |OFF |ON...
       └──┘  └──┘└──┘  └───┘ └──┘
       300m 200m 100m 400m  650m  (循环)
       
特点: "咚--哒-咚---哒-" 复杂节奏
适用: 来电铃声、闹钟、音乐节拍
```

##### **模式7: RAMP_UP（渐强）**
```
时间:  0    50  100  150  200  ...  2500 (ms)
强度:  0%   4%  8%  12% 16%  ...  100%(重置)
马达:  弱──→──→──→──→──→──→──→──→ 强 ──┐
                                                  │ (循环)
                                              重置为弱
                                              
特点: 类似"呼吸灯"的振动版本
参数: 每50ms增加2%，达到100%后重置
适用: 游戏中的"蓄力"效果、渐进式唤醒
```

##### **模式8: RAMP_DOWN（渐弱）**
```
时间:  0    50  100  150  200  ...  2500 (ms)
强度: 100% 96% 92% 88% 84%  ...  0%(重置)
马达:  强──→──→──→──→──→──→──→ 弱 ──┐
                                          │ (循环)
                                      重置为强
                                      
特点: 与RAMP_UP对称，平缓停止
参数: 每50ms减少2%，达到0%后重置
适用: 提示结束、温柔停止
```

##### **模式9: RANDOM（随机）**
```
时间:  0   ?    ?    ?    ?    ? (ms)
马达:  |ON |OFF |ON  |OFF |ON...
       └─┘  └──┘  └──┘  └──┘
       随机  随机  随机  随机
       
振动时间: 100ms ~ 299ms (随机)
停止时间: 100ms ~ 399ms (随机)
种子: 线性同余算法 (可预测但看似随机)

特点: 不可预测，有趣味性
适用: 游戏反馈、彩蛋、防疲劳
```

#### 使用示例
```c
/* 示例1：基本用法 */
#include "motor.h"

void setup(void) {
    Motor_Init();
    Motor_SetMode(MOTOR_MODE_PATTERN_1);  /* 设置为模式4 */
}

void loop(void) {
    Motor_Poll();  /* 必须1ms调用一次 */
    Delay_ms(1);
}

/* 示例2：循环切换所有模式 */
void demo_all_modes(void) {
    Motor_Mode_t mode;
    
    for (mode = MOTOR_MODE_IDLE; mode < MOTOR_MODE_COUNT; mode++) {
        Motor_SetMode(mode);
        
        /* 运行当前模式5秒钟 */
        for (int i = 0; i < 5000; i++) {
            Motor_Poll();
            Delay_ms(1);
        }
    }
}

/* 示例3：自定义振动序列 */
void custom_sequence(void) {
    /* 三次短振 + 一次长振 */
    Motor_Control(1); Delay_ms(100);
    Motor_Control(0); Delay_ms(100);
    Motor_Control(1); Delay_ms(100);
    Motor_Control(0); Delay_ms(100);
    Motor_Control(1); Delay_ms(100);
    Motor_Control(0); Delay_ms(500);
    Motor_Control(1); Delay_ms(500);
    Motor_Control(0);
}
```

#### 自定义新模式（扩展指南）

**步骤1：在motor.h中添加枚举值**
```c
typedef enum {
    MOTOR_MODE_IDLE,
    /* ... 现有模式 ... */
    MOTOR_MODE_RANDOM,
    MOTOR_MODE_CUSTOM_NEW,  /* ← 新增 */
    MOTOR_MODE_COUNT
} Motor_Mode_t;
```

**步骤2：在motor.c中实现函数**
```c
/**
 * @brief  自定义新模式：双击+超长停
 * @note   时序：100ms振-100ms停-100ms振-1000ms停
 * @适用场景: 特殊定制需求
 */
void Motor_Mode_Custom_New(void)
{
    if (s_pattern_timer == 0) {
        switch (s_pattern_step) {
            case 0:
                Motor_Control(1);
                s_pattern_timer = 100;
                break;
            case 1:
                Motor_Control(0);
                s_pattern_timer = 100;
                break;
            case 2:
                Motor_Control(1);
                s_pattern_timer = 100;
                break;
            case 3:
                Motor_Control(0);
                s_pattern_timer = 1000;
                break;
            default:
                break;
        }
        s_pattern_step = (s_pattern_step + 1) % 4;
    } else {
        s_pattern_timer--;
    }
}
```

**步骤3：在Motor_Poll()中注册**
```c
case MOTOR_MODE_CUSTOM_NEW:
    Motor_Mode_Custom_New();
    break;
```

**完成！** 新模式已经可用。

---

### 7.4 电源管理

#### 功耗模式对比

| 模式 | 典型电流 | CPU状态 | 外设状态 | 唤醒方式 |
|------|---------|---------|---------|----------|
| **正常运行** | ~5MHz | 全速运行 | 全部活跃 | - |
| **STOP模式** | **< 7μA** | 停止 | 大部分关闭 | INT0/INT4 |
| **深度睡眠** | < 1μA | 完全停止 | 全部关闭 | 复位/RTC |

#### STOP模式工作原理
```
正常模式:
┌─────────────────────────────────────────┐
│ CPU: 运行中 (12MHz)                      │
│ 外设: GPIO/Timer/UART 全部开启           │
│ 电流: ~5mA                               │
│ 功能: 完整功能可用                        │
└─────────────────────────────────────────┘
                    │ 用户操作/充电器拔出/超时
                    ▼
STOP模式:
┌─────────────────────────────────────────┐
│ CPU: 停止 (时钟关闭)                      │
│ 外设: 仅INT0/INT4保持活跃                │
│ 电流: < 7μA (节省99.86%)                 │
│ 功能: 等待唤醒                            │
│ 唤醒源:                                  │
│   · P0.0 按键下降沿 (INT0)               │
│   · P0.5 充电器变化 (INT4)               │
└─────────────────────────────────────────┘
                    │ 检测到唤醒信号
                    ▼
恢复正常:
┌─────────────────────────────────────────┐
│ CPU: 恢复运行                             │
│ 外设: 全部恢复正常                        │
│ 电流: ~5mA                               │
│ 功能: 继续正常工作                        │
└─────────────────────────────────────────┘
```

#### 自动关机机制
```
条件检查（每1ms执行一次）:
┌─────────────────────────────────────┐
│ 1. 系统是否处于ON状态？              │──NO──→ 跳过
│    │YES                             │
│ ▼                                   │
│ 2. 是否有充电器连接？               │──YES─→ 跳过
│    │NO                              │
│ ▼                                   │
│ 3. 马达是否处于IDLE模式？           │──NO──→ 跳过
│    │YES                             │
│ ▼                                   │
│ 4. 空闲时间 >= 5分钟？              │──NO──→ 计时+1ms
│    │YES                             │
│ ▼                                   │
│ 执行关机！                           │
│ · LED熄灭                           │
│ · 马达停止                          │
│ · 进入STOP模式                      │
└─────────────────────────────────────┘
```

#### 充电器检测流程
```
充电器插入:
P0.5: LOW → HIGH (上升沿)
         │
         ▼
INT4中断触发
         │
         ▼
读取P0.1 (充电状态):
├─ LOW  → CHARGER_STATE_CHARGING (充电中)
└─ HIGH → CHARGER_STATE_FULL (已充满)
         │
         ▼
如果系统处于OFF → 自动开机
更新LED状态 (常亮/闪烁)


充电器拔出:
P0.5: HIGH → LOW (下降沿)
         │
         ▼
INT4中断触发
         │
         ▼
CHARGER_STATE_NONE (无充电器)
         │
         ▼
如果系统处于ON → 自动关机 (进入STOP模式)
```

#### 使用示例
```c
/* 手动进入低功耗 */
void enter_low_power(void) {
    /* 保存关键数据到非易失存储（如果需要） */
    save_context();
    
    /* 关闭外设 */
    Motor_Control(0);
    LED_SetState(LED_STATE_OFF);
    
    /* 进入STOP模式（此处阻塞，直到被唤醒）*/
    Power_EnterStopMode();
    
    /* 被唤醒后继续执行 */
    Power_WakeUpHandler();
    
    /* 恢复系统 */
    LED_SetState(LED_STATE_ON);
}

/* 检查电池状态 */
void check_battery(void) {
    Charger_State_t charger = Power_GetChargerState();
    
    switch (charger) {
        case CHARGER_STATE_CHARGING:
            printf("正在充电...\n");
            LED_SetState(LED_STATE_BLINK_CHARGING);
            break;
            
        case CHARGER_STATE_FULL:
            printf("充电完成！\n");
            LED_SetState(LED_STATE_ON);
            break;
            
        case CHARGER_STATE_NONE:
            printf("未连接充电器\n");
            if (battery_low()) {
                printf("电量不足，即将关机\n");
                Power_SetState(POWER_STATE_OFF);
            }
            break;
    }
}
```

#### 配置参数
```c
#define AUTO_POWEROFF_TIME_MS  300000  /* 5分钟 = 300000ms */
#define STARTUP_VIBRATION_MS   50      /* 开机确认振动时间 */
```

---

## 8. 配置参数说明

### 8.1 系统级配置

```c
/* ==================== 时钟配置 ==================== */
#define F_CPU  12000000UL  
/*
 * CPU时钟频率(Hz)
 * 可选值:
 *   12000000UL  (12.0 MHz) - 默认，常用
 *   11059200UL  (11.0592 MHz) - 串口通信常用
 *   8000000UL   (8.0 MHz)  - 低功耗
 *   16000000UL  (16.0 MHz) - 高性能
 *   
 * 修改此值后，Timer0重载值会自动适配
 */
```

### 8.2 引脚配置

```c
/* ==================== 按键引脚 ==================== */
#define BUTTON_PIN  GPIO_PIN_0
/*
 * 按键连接的GPIO引脚
 * 可选: GPIO_PIN_0 ~ GPIO_PIN_5
 * 注意: 必须是支持外部中断的引脚(P0.0用于INT0)
 */

/* ==================== LED引脚 ==================== */
#define LED_PIN  GPIO_PIN_2
/*
 * LED连接的GPIO引脚
 * 可选: GPIO_PIN_0 ~ GPIO_PIN_5
 * 要求: 必须是推挽输出能力
 */

/* ==================== 马达引脚 ==================== */
#define MOTOR_PIN  GPIO_PIN_3
/*
 * 马达MOSFET栅极连接的引脚
 * 可选: GPIO_PIN_0 ~ GPIO_PIN_5
 * 要求: 驱动能力足够(>10mA)
 */

/* ==================== 充电相关引脚 ==================== */
#define CHARGER_DETECT_PIN  GPIO_PIN_5
/*
 * 充电器检测引脚（经过分压电阻）
 * 重要: 此引脚不需要内部上下拉！
 *       由外部电路提供分压
 */

#define CHARGE_STATUS_PIN  GPIO_PIN_1
/*
 * 充电IC状态输出引脚
 * 电平: LOW=充电中, HIGH=充满
 * 需要: 内部上拉
 */
```

### 8.3 时间参数配置

```c
/* ==================== 按键时间参数 ==================== */
#define BUTTON_DEBOUNCE_MS    20
/*
 * 按键消抖时间(ms)
 * 范围: 10~50ms
 * 建议: 20ms (机械按键标准值)
 * 过小: 可能误触
 *过大: 响应迟钝
 */

#define BUTTON_LONG_PRESS_MS  1500
/*
 * 长按判定阈值(ms)
 * 范围: 1000~3000ms
 * 建议: 1500ms (1.5秒，用户体验好)
 * 过小: 容易误触发长按
 *过大: 手指疲劳
 */

/* ==================== LED时间参数 ==================== */
#define LED_BLINK_CHARGING_PERIOD  250
/*
 * 充电闪烁半周期(ms)
 * 完整周期 = 2 × 250 = 500ms
 * 即: 250ms亮 + 250ms灭 = 每秒闪2次
 * 调整: 改此值改变闪烁频率
 */

#define LED_BLINK_500MS_DURATION  500
/*
 * 短按模式切换时的LED熄灭时间(ms)
 * 用途: 给用户的视觉反馈"模式已切换"
 * 建议: 300~800ms
 */

/* ==================== 马达时间参数 ==================== */
/* 模式4: PATTERN_1 */
#define MOTOR_VIBRATE_200MS  200  /* 振动时间 */
#define MOTOR_STOP_100MS     100  /* 停止时间 */

/* 模式5: PATTERN_2 */
#define MOTOR_VIBRATE_100MS  100  /* 振动时间 */

/* 模式6: PATTERN_3 */
#define MOTOR_VIBRATE_300MS  300  /* 长振动 */
#define MOTOR_STOP_200MS     200  /* 中停 */
#define MOTOR_VIBRATE_150MS  150  /* 短振动 */
#define MOTOR_STOP_400MS     400  /* 长停 */
#define MOTOR_STOP_650MS     650  /* 超长停 */

/* 模式7/8: 渐变参数 */
#define RAMP_STEP               2    /* 每次调整的步进值 */
#define RAMP_UPDATE_INTERVAL_MS 50   /* 更新间隔(ms) */
#define RAMP_MAX_DUTY           100  /* 最大占空比(%) */

/* 模式9: 随机参数 */
#define RANDOM_VIBRATE_MIN_MS  100  /* 最短振动时间 */
#define RANDOM_VIBRATE_MAX_MS  299  /* 最长振动时间 */
#define RANDOM_STOP_MIN_MS     100  /* 最短停止时间 */
#define RANDOM_STOP_MAX_MS     399  /* 最长停止时间 */

/* ==================== 电源管理参数 ==================== */
#define AUTO_POWEROFF_TIME_MS  300000UL
/*
 * 自动关机时间(ms)
 * 300000ms = 300秒 = 5分钟
 * 建议: 300000~600000 (5~10分钟)
 * 设为0: 禁用自动关机
 */

#define STARTUP_VIBRATION_MS  50
/*
 * 开机确认振动时间(ms)
 * 用途: 让用户感知到"已开机"
 * 建议: 30~100ms
 */

/* ==================== 中断向量配置 ==================== */
#define INTERRUPT_VECTOR_BUTTON  0
/*
 * 按键中断向量号 (INT0)
 * CA51M550固定为0
 * 不要修改！
 */

#define INTERRUPT_VECTOR_CHARGER 6
/*
 * 充电器检测中断向量号 (INT4)
 * CA51M550固定为6
 * 不要修改！
 */
```

### 8.4 如何自定义配置

#### 示例：调整为快速响应版本
```c
/* 在config.h中修改 */

#define BUTTON_DEBOUNCE_MS    10       /* 更快响应 */
#define BUTTON_LONG_PRESS_MS  1000     /* 1秒即判定长按 */
#define AUTO_POWEROFF_TIME_MS 120000   /* 2分钟自动关机 */
#define LED_BLINK_500MS_DURATION 200   /* 更快的闪烁反馈 */
```

#### 示例：调整为省电版本
```c
#define BUTTON_DEBOUNCE_MS    30       /* 更稳健的消抖 */
#define BUTTON_LONG_PRESS_MS  2000     /* 2秒才判定长按 */
#define AUTO_POWEROFF_TIME_MS 600000   /* 10分钟自动关机 */
/* 降低马达振动强度（通过缩短振动时间）*/
#define MOTOR_VIBRATE_200MS  150       /* 减少振动时间 */
```

---

## 9. API参考手册

### 9.1 GPIO模块API

#### `void GPIO_Init(GPIO_Pin_t pin, GPIO_Mode_t mode)`
**功能**: 初始化GPIO引脚

**参数**:
- `pin`: 引脚号 (GPIO_PIN_0 ~ GPIO_PIN_5)
- `mode`: 模式
  - `GPIO_MODE_INPUT` - 浮空输入
  - `GPIO_MODE_OUTPUT` - 推挽输出
  - `GPIO_MODE_INPUT_PULLUP` - 上拉输入
  - `GPIO_MODE_INPUT_ANALOG` - 模拟输入（无上下拉）

**返回值**: 无

**示例**:
```c
GPIO_Init(GPIO_PIN_2, GPIO_MODE_OUTPUT);        /* LED */
GPIO_Init(GPIO_PIN_0, GPIO_MODE_INPUT_PULLUP);   /* 按键 */
GPIO_Init(GPIO_PIN_5, GPIO_MODE_INPUT_ANALOG);   /* 分压输入 */
```

---

#### `void GPIO_WritePin(GPIO_Pin_t pin, GPIO_Level_t level)`
**功能**: 设置GPIO输出电平

**参数**:
- `pin`: 引脚号
- `level`: 电平
  - `GPIO_LEVEL_LOW` - 低电平 (0V)
  - `GPIO_LEVEL_HIGH` - 高电平 (VCC)

**返回值**: 无

**示例**:
```c
GPIO_WritePin(GPIO_PIN_2, GPIO_LEVEL_HIGH);  /* 点亮LED */
GPIO_WritePin(GPIO_PIN_2, GPIO_LEVEL_LOW);   /* 熄灭LED */
```

---

#### `GPIO_Level_t GPIO_ReadPin(GPIO_Pin_t pin)`
**功能**: 读取GPIO输入电平

**参数**:
- `pin`: 引脚号

**返回值**:
- `GPIO_LEVEL_LOW` - 读到低电平
- `GPIO_LEVEL_HIGH` - 读到高电平

**示例**:
```c
if (GPIO_ReadPin(GPIO_PIN_0) == GPIO_LEVEL_LOW) {
    /* 按键被按下 */
}
```

---

#### `void GPIO_TogglePin(GPIO_Pin_t pin)`
**功能**: 翻转GPIO电平

**参数**:
- `pin`: 引脚号

**返回值**: 无

**示例**:
```c
GPIO_TogglePin(GPIO_PIN_2);  /* LED翻转 */
```

---

### 9.2 按键模块API

#### `void Button_Init(void)`
**功能**: 初始化按键模块

**配置**:
- P0.0设置为输入+内部上拉
- 使能INT0中断（下降沿触发）
- 初始化内部状态机

**调用时机**: 系统启动时调用一次

**示例**:
```c
void setup(void) {
    Button_Init();
}
```

---

#### `void Button_Poll(void)`
**功能**: 按键轮询（非阻塞）

**调用频率**: 必须**每1ms调用一次**

**内部功能**:
- 读取按键电平
- 20ms软件消抖
- 判断按下时长
- 生成按键事件

**重要**: 此函数必须在定时器中断或主循环的高频调用

**示例**:
```c
void loop(void) {
    Button_Poll();  /* 每1ms调用 */
    Delay_ms(1);
}
```

---

#### `Button_Event_t Button_GetEvent(void)`
**功能**: 获取最新的按键事件

**返回值**:
- `BUTTON_EVENT_NONE` - 无事件
- `BUTTON_EVENT_SHORT_PRESS` - 短按（< 1.5秒）
- `BUTTON_EVENT_LONG_PRESS` - 长按（≥ 1.5秒）

**注意**: 事件会被保留直到被清除

**示例**:
```c
switch (Button_GetEvent()) {
    case BUTTON_EVENT_SHORT_PRESS:
        handle_short_press();
        break;
    case BUTTON_EVENT_LONG_PRESS:
        handle_long_press();
        break;
}
```

---

#### `void Button_ClearEvent(void)`
**功能**: 清除当前的按键事件

**调用时机**: 处理完事件后调用

**示例**:
```c
if (Button_GetEvent() != BUTTON_EVENT_NONE) {
    process_event();
    Button_ClearEvent();  /* 清除，避免重复处理 */
}
```

---

#### `uint8_t Button_IsPressed(void)`
**功能**: 查询按键当前是否被按下

**返回值**:
- `1` - 当前按下
- `0` - 当前释放

**注意**: 此函数**未消抖**，直接读取引脚状态

**示例**:
```c
if (Button_IsPressed()) {
    /* 按键正被按住 */
}
```

---

### 9.3 LED模块API

#### `void LED_Init(void)`
**功能**: 初始化LED模块

**配置**:
- P0.2设置为输出
- 初始状态：关闭

**示例**:
```c
LED_Init();
```

---

#### `void LED_SetState(LED_State_t state)`
**功能**: 设置LED显示状态

**参数**:
- `LED_STATE_OFF` - 关闭
- `LED_STATE_ON` - 常亮
- `LED_STATE_BLINK_CHARGING` - 500ms周期闪烁
- `LED_STATE_BLINK_500MS` - 熄灭500ms后恢复

**副作用**: 会重置内部计时器

**示例**:
```c
LED_SetState(LED_STATE_ON);              /* 常亮 */
LED_SetState(LED_STATE_BLINK_CHARGING); /* 充电闪烁 */
```

---

#### `void LED_Poll(void)`
**功能**: LED状态机轮询（非阻塞）

**调用频率**: 必须**每1ms调用一次**

**内部功能**:
- 处理闪烁计时
- 更新LED输出
- 状态转换

**示例**:
```c
void loop(void) {
    LED_Poll();
    Delay_ms(1);
}
```

---

#### `LED_State_t LED_GetState(void)`
**功能**: 获取LED当前状态

**返回值**: 当前的LED状态枚举值

**示例**:
```c
if (LED_GetState() == LED_STATE_OFF) {
    /* LED当前是关闭的 */
}
```

---

#### `void LED_StartBlink500ms(void)`
**功能**: 启动500ms熄灭模式

**用途**: 短按切换模式时的视觉反馈

**行为**:
1. 立即熄灭LED
2. 保持熄灭500ms
3. 500ms后自动恢复为常亮(ON)状态

**示例**:
```c
void on_button_short_press(void) {
    Motor_NextMode();
    LED_StartBlink500ms();  /* 闪烁一下提示用户 */
}
```

---

### 9.4 马达模块API

#### `void Motor_Init(void)`
**功能**: 初始化马达模块

**配置**:
- P0.3设置为输出
- 初始状态：关闭
- 模式：IDLE

**示例**:
```c
Motor_Init();
```

---

#### `void Motor_Control(uint8_t enable)`
**功能**: 直接控制马达开关

**参数**:
- `1` / 非0 - 开启马达
- `0` - 关闭马达

**注意**: 这会**覆盖**当前模式的状态

**示例**:
```c
Motor_Control(1);  /* 开启 */
Delay_ms(500);
Motor_Control(0);  /* 关闭 */
```

---

#### `void Motor_SetMode(Motor_Mode_t mode)`
**功能**: 设置马达振动模式

**参数**:
- `MOTOR_MODE_IDLE` (0) - 空闲
- `MOTOR_MODE_CONTINUOUS` (1) - 持续
- `MOTOR_MODE_INTERVAL_1S` (2) - 1秒周期
- `MOTOR_MODE_INTERVAL_2S` (3) - 2秒周期
- `MOTOR_MODE_PATTERN_1` (4) - 节奏1
- `MOTOR_MODE_PATTERN_2` (5) - 节奏2
- `MOTOR_MODE_PATTERN_3` (6) - 节奏3
- `MOTOR_MODE_RAMP_UP` (7) - 渐强
- `MOTOR_MODE_RAMP_DOWN` (8) - 渐弱
- `MOTOR_MODE_RANDOM` (9) - 随机

**副作用**:
- 重置模式内部状态（定时器、步骤、占空比）
- 如果设置为IDLE，会立即关闭马达

**示例**:
```c
Motor_SetMode(MOTOR_MODE_PATTERN_1);  /* 设置为节奏模式1 */
```

---

#### `Motor_Mode_t Motor_GetMode(void)`
**功能**: 获取当前马达模式

**返回值**: 当前模式枚举值

**示例**:
```c
if (Motor_GetMode() == MOTOR_MODE_IDLE) {
    /* 马达处于空闲模式 */
}
```

---

#### `void Motor_Poll(void)`
**功能**: 马达状态机轮询（非阻塞）

**调用频率**: 必须**每1ms调用一次**

**内部功能**:
- 根据当前模式调用对应的模式函数
- 更新定时器和状态
- 控制马达开关

**示例**:
```c
void loop(void) {
    Motor_Poll();
    Delay_ms(1);
}
```

---

### 9.5 电源管理API

#### `void Power_Init(void)`
**功能**: 初始化电源管理模块

**配置**:
- P0.5设置为模拟输入（充电器检测）
- P0.1设置为上拉输入（充电状态）
- 使能INT4中断（双沿触发）
- 初始化状态为OFF

**示例**:
```c
Power_Init();
```

---

#### `Power_State_t Power_GetState(void)`
**功能**: 获取电源状态

**返回值**:
- `POWER_STATE_OFF` - 关机/STOP模式
- `POWER_STATE_ON` - 开机/运行模式

**示例**:
```c
if (Power_GetState() == POWER_STATE_ON) {
    /* 系统正在运行 */
}
```

---

#### `Charger_State_t Power_GetChargerState(void)`
**功能**: 获取充电器状态

**返回值**:
- `CHARGER_STATE_NONE` - 无充电器
- `CHARGER_STATE_CHARGING` - 充电中
- `CHARGER_STATE_FULL` - 已充满

**示例**:
```c
switch (Power_GetChargerState()) {
    case CHARGER_STATE_CHARGING:
        /* 正在充电 */
        break;
    case CHARGER_STATE_FULL:
        /* 充满电 */
        break;
    default:
        /* 未连接充电器 */
        break;
}
```

---

#### `void Power_SetState(Power_State_t state)`
**功能**: 设置电源状态

**参数**:
- `POWER_STATE_ON` - 开机（同时重置空闲计时器）
- `POWER_STATE_OFF` - 关机

**示例**:
```c
Power_SetState(POWER_STATE_ON);   /* 开机 */
Power_SetState(POWER_STATE_OFF);  /* 关机 */
```

---

#### `uint32_t Power_GetIdleCounter(void)`
**功能**: 获取空闲计时器值

**返回值**: 空闲时间（毫秒）

**示例**:
```c
uint32_t idle_time = Power_GetIdleCounter();
printf("已空闲 %lu ms\n", idle_time);
```

---

#### `void Power_ResetIdleCounter(void)`
**功能**: 重置空闲计时器

**调用时机**: 检测到用户操作时调用，防止自动关机

**示例**:
```c
void on_user_action(void) {
    /* 处理用户操作... */
    Power_ResetIdleCounter();  /* 重置，重新计时 */
}
```

---

#### `void Power_EnterStopMode(void)`
**功能**: 进入STOP低功耗模式

**功耗**: < 7μA

**阻塞**: 是！此函数会**阻塞**直到被唤醒

**唤醒源**:
- INT0 (P0.0 按键)
- INT4 (P0.5 充电器变化)

**示例**:
```c
void go_to_sleep(void) {
    /* 保存必要的上下文... */
    
    Power_EnterStopMode();  /* 阻塞在此，等待唤醒 */
    
    /* 被唤醒后继续执行 */
    Power_WakeUpHandler();
    
    /* 恢复系统... */
}
```

---

#### `void Power_WakeUpHandler(void)`
**功能**: STOP模式唤醒后的恢复处理

**功能**:
- 恢复系统时钟
- 重新使能全局中断

**调用时机**: `Power_Return()`之后立即调用

**示例**:
```c
Power_EnterStopMode();
Power_WakeUpHandler();  /* 必须调用！ */
```

---

#### `void Power_Poll(void)`
**功能**: 电源管理轮询（非阻塞）

**调用频率**: 建议**每1ms调用一次**

**内部功能**:
- 检测充电器插拔
- 更新充电状态
- 检查是否需要自动关机

**示例**:
```c
void loop(void) {
    Power_Poll();
    Delay_ms(1);
}
```

---

## 10. 移植指南

### 10.1 移植到其他8051兼容MCU

**步骤**:

1️⃣ **修改ca51m550.h**
```c
/* 替换为目标MCU的寄存器定义 */
/* 例如: stc15.h, at89c51.h 等 */
```

2️⃣ **修改gpio.c**（唯一需要改动的业务无关文件）
```c
static uint8_t code s_gpio_pin_reg[] = {
    /* 目标MCU的端口配置寄存器地址 */
};

void GPIO_Init(GPIO_Pin_t pin, GPIO_Mode_t mode) {
    /* 目标MCU的GPIO初始化代码 */
}

void GPIO_WritePin(GPIO_Pin_t pin, GPIO_Level_t level) {
    /* 目标MCU的GPIO写操作 */
}

GPIO_Level_t GPIO_ReadPin(GPIO_Pin_t pin) {
    /* 目标MCU的GPIO读操作 */
    return level;
}
```

3️⃣ **修改config.h**
```c
/* 调整引脚分配（如果目标MCU引脚不同）*/
#define BUTTON_PIN  GPIO_PIN_xxx
/* ... */

/* 调整时钟频率 */
#define F_CPU  11059200UL  /* 例如 */

/* 调整中断向量（如果不同）*/
#define INTERRUPT_VECTOR_BUTTON  xx
#define INTERRUPT_VECTOR_CHARGER xx
```

4️⃣ **其他文件无需修改！** ✅

---

### 10.2 移植到ARM Cortex-M (STM32/ESP32等)

**步骤**:

1️⃣ **重写gpio.c**（使用HAL库）
```c
/* gpio.c - STM32版本 */
#include "stm32f1xx_hal.h"

void GPIO_Init(GPIO_Pin_t pin, GPIO_Mode_t mode) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = (1 << pin);
    
    switch (mode) {
        case GPIO_MODE_OUTPUT:
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            break;
        case GPIO_MODE_INPUT_PULLUP:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            break;
        /* ... 其他模式 */
    }
    
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  /* 假设用GPIOA */
}

void GPIO_WritePin(GPIO_Pin_t pin, GPIO_Level_t level) {
    HAL_GPIO_WritePin(GPIOA, (1 << pin), 
                      (level == GPIO_LEVEL_HIGH) ? 
                      GPIO_PIN_SET : GPIO_PIN_RESET);
}

GPIO_Level_t GPIO_ReadPin(GPIO_Pin_t pin) {
    return (HAL_GPIO_ReadPin(GPIOA, (1 << pin)) == GPIO_PIN_SET) ?
           GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
}
```

2️⃣ **修改中断服务程序**
```c
/* button.c - STM32版本 */
void EXTI0_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0) != RESET) {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
        s_button_event = BUTTON_EVENT_SHORT_PRESS;
    }
}
```

3️⃣ **修改Timer配置**
```c
/* main.c - STM32版本 */
void Timer_Init(void) {
    /* 使用TIM2作为1ms基准定时器 */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 8400-1;  /* 84MHz/8400 = 10kHz */
    htim2.Init.CounterPeriod = 10-1; /* 10kHz/10 = 1kHz (1ms) */
    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_Base_Start_IT(&htim2);
}

void TIM2_IRQHandler(void) {
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) != RESET) {
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
        s_1ms_tick++;
        /* ... 更新空闲计数器 ... */
    }
}
```

4️⃣ **修改STOP模式实现**
```c
/* power.c - STM32版本 */
void Power_EnterStopMode(void) {
    /* 进入STOP模式 */
    HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
}

void Power_WakeUpHandler(void) {
    /* 唤醒后重新配置时钟 */
    SystemClock_Config();
}
```

5️⃣ **button.c / led.c / motor.c / power.c / main.c 的业务逻辑保持不变！** ✅

---

### 10.3 移植检查清单

- [ ] 1. 更新ca51m550.h 为目标MCU头文件
- [ ] 2. 重写gpio.c（GPIO操作）
- [ ] 3. 修改config.h（引脚、时钟、中断向量）
- [ ] 4. 更新中断服务程序（向量号和寄存器）
- [ ] 5. 重新配置Timer（如果目标MCU不同）
- [ ] 6. 实现STOP模式（如果支持低功耗）
- [ ] 7. 测试所有10种马达模式
- [ ] 8. 测试按键长短按
- [ ] 9. 测试LED所有状态
- [ ] 10. 测试充电器插拔和自动关机
- [ ] 11. 测量STOP模式功耗
- [ ] 12. 长时间稳定性测试

---

## 11. 故障排除

### 11.1 编译问题

#### 问题1：找不到头文件
**错误**: `Fatal Error: Cannot open file 'config.h'`

**原因**: Include路径未正确设置

**解决**:
```
Keil Options → C51 → Include Paths:
添加: 你的项目绝对路径
例如: D:\tools\ca51m550\project
```

---

#### 问题2：语法错误
**错误**: `syntax error near 'for'`

**原因**: 使用了C99/C11语法（如for循环内声明变量）

**解决**:
```c
/* ❌ 错误 (C99) */
for (int i = 0; i < 10; i++) { }

/* ✅ 正确 (C89) */
int i;
for (i = 0; i < 10; i++) { }
```

---

#### 问题3：RAM溢出
**错误**: `L107: address space overflow`

**原因**: 变量占用过多RAM

**解决**:
1. 减少全局变量
2. 使用 `code` 关键字将常量放入ROM
3. 优化数据类型（如uint8_t代替uint16_t）

**示例**:
```c
/* 优化前 */
uint8_t table[256];  /* 占用256字节RAM */

/* 优化后 */
uint8_t code table[256] = {...};  /* 存储在ROM，不占RAM */
```

---

### 11.2 运行时问题

#### 问题1：按键无反应
**现象**: 按下按键没有任何反应

**排查步骤**:
```
1. 检查硬件连接
   └─ 万用表测量P0.0电压
      · 释放时应≈VCC (上拉)
      · 按下时应≈0V

2. 检查初始化顺序
   └─ 确认Button_Init()已被调用

3. 检查中断使能
   └─ EA = 1 是否执行？
   └─ EX0 = 1 是否设置？

4. 调试方法
   └─ 在Button_Poll()入口处设置断点
   └─ 观察GPIO_ReadPin()返回值
```

**常见原因**:
- ❌ 忘记调用 `Button_Init()`
- ❌ 忘记全局中断使能 `EA = 1`
- ❌ 按键硬件损坏或接触不良
- ❌ 上拉电阻未生效（检查P00F配置）

---

#### 问题2：马达不振动
**现象**: 设置了模式但马达不转

**排查步骤**:
```
1. 直接测试GPIO
   └─ 在main()开头添加:
      GPIO_WritePin(MOTOR_PIN, GPIO_LEVEL_HIGH);
      如果马达转动 → 问题在Motor模块
      如果不转 → 硬件问题

2. 检查Motor_Poll()是否被调用
   └─ 应该每1ms调用一次

3. 检查当前模式
   └─ Motor_GetMode() 是否等于期望的模式？

4. 检查MOSFET电路
   └─ 测量Gate电压
   └─ 检查VCC是否充足
   └─ 马达本身是否损坏
```

**常见原因**:
- ❌ 忘记调用 `Motor_Poll()`
- ❌ 当前模式是 `MOTOR_MODE_IDLE`
- ❌ MOSFET栅极驱动不足
- ❌ 电池电压过低
- ❌ 马达损坏或卡住

---

#### 问题3：LED不亮或常亮
**现象**: LED状态不符合预期

**排查步骤**:
```
1. 直接测试LED
   └─ GPIO_WritePin(LED_PIN, GPIO_LEVEL_HIGH);
      如果亮 → LED硬件OK
      如果不亮 → 检查LED极性和限流电阻

2. 检查LED_Poll()调用
   └─ 是否每1ms调用？

3. 检查LED状态
   └─ LED_GetState() 返回什么？

4. 检查优先级逻辑
   └─ 是否被充电状态覆盖？
```

**常见原因**:
- ❌ LED极性接反（阳极/阴极）
- ❌ 限流电阻太大（建议220Ω~1kΩ）
- ❌ 忘记调用 `LED_Poll()`
- ❌ 充电状态覆盖了手动设置的ON状态

---

#### 问题4：无法进入STOP模式
**现象**: 调用Power_EnterStopMode()后电流仍然很大

**排查步骤**:
```
1. 检查外围设备
   └─ LED是否关闭？
   └─ 马达是否关闭？
   └─ 其他外设是否关闭？

2. 检查中断配置
   └─ 是否有不必要的中断源仍在运行？
   └─ Timer是否停止？

3. 检查GPIO状态
   └─ 避免浮空输入（会产生额外电流）
   └─ 未使用的引脚应设为输出低/上拉输入

4. 测量电流
   └─ 使用高精度电流表（μA级）
   └─ 对比数据手册的典型值
```

**常见原因**:
- ❌ LED/MOSFET未完全关闭
- ❌ 有其他外设仍在消耗电流
- ❌ GPIO浮空输入产生漏电流
- ❌ 未调用正确的STOP模式指令

---

#### 问题5：充电器检测不准确
**现象**: 插拔充电器没有反应，或状态错误

**排查步骤**:
```
1. 检查分压电路
   └─ 充电器插入时，P0.5电压应为2~3V
   └─ 充电器拔出时，P0.5电压应为0V

2. 检查中断配置
   └─ INT4是否使能 (INT4EN = 1)?
   └─ EP2CON是否配置为双沿触发？

3. 直接读取引脚
   └─ GPIO_ReadPin(CHARGER_DETECT_PIN) 返回值?

4. 检查充电IC状态引脚
   └─ P0.1的电平是否符合预期？
```

**常见原因**:
- ❌ 分压电阻值不对
- ❌ P0.5错误地配置了内部上拉
- ❌ INT4中断未使能
- ❌ 充电IC状态引脚电平与预期相反

---

### 11.3 功耗问题

#### 问题1：STOP模式电流过大（> 100μA）
**预期**: < 7μA  
**实际**: > 100μA

**排查和解决**:
```c
/* 1. 确保所有GPIO正确配置 */
void prepare_for_stop(void) {
    /* 关闭LED */
    LED_SetState(LED_STATE_OFF);
    
    /* 关闭马达 */
    Motor_Control(0);
    
    /* 未使用的引脚设为输出低或上拉输入 */
    GPIO_WritePin(GPIO_PIN_4, GPIO_LEVEL_LOW);
    
    /* 关闭ADC/其他外设 */
    ADC_DISABLE();
    UART_DISABLE();
}

/* 2. 检查是否有未关闭的外设 */
/* 3. 检查PCB漏电（清洗板子）*/
/* 4. 使用电流表串联测量 */
```

---

## 12. 版本历史

| 版本 | 日期 | 作者 | 变更内容 |
|------|------|------|----------|
| v1.0 | 2026-09-28 | AI Assistant | 初始版本 |
| v1.1 | 2026-09-28 | AI Assistant | 添加GPIO HAL层 |
| v1.2 | 2026-09-28 | AI Assistant | 完善文档注释+工程指南 |

---

## 📞 技术支持

如遇到问题，请按以下顺序排查：

1. **查阅本指南** - 第11章"故障排除"
2. **检查硬件连接** - 第2章"硬件规格"
3. **查看示例代码** - 各API的使用示例
4. **阅读源码注释** - 所有函数都有详细注释

---

## 📄 许可证

本项目仅供学习和参考使用。

---

**🎉 感谢使用 CA51M550 马达振动控制器！**

*最后更新: 2026-09-28*