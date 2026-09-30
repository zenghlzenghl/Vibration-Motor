#include "ca51m550.h"
#include "config.h"
#include "button.h"
#include "led.h"
#include "motor.h"
#include "power.h"
#include "app.h"

volatile u16 s_1ms_tick = 0;
volatile bit s_1ms_flag = 0;

static void Timer0_Init(void);
static void System_Init(void);

/**
 * @brief  Timer0初始化 - 1ms定时中断
 * @note   使用模式1（16位定时器）
 *         定时器重载值由F_CPU自动计算
 */
static void Timer0_Init(void)
{
    TMOD &= 0xF0;
    TMOD |= 0x01;
    TH0 = TIMER0_RELOAD_H;
    TL0 = TIMER0_RELOAD_L;
    ET0 = 1;
    TR0 = 1;
}

/**
 * @brief  Timer0中断服务程序 - 系统时钟基准
 * @note   每1ms执行一次（由硬件定时器精确保证）
 *         功能：
 *         - 更新系统1ms计时器
 *         - 设置1ms调度标志位（供主循环使用）
 *         - 更新空闲计时器（用于自动关机判断）
 */
void Timer0_ISR(void) interrupt 1
{
    TH0 = TIMER0_RELOAD_H;
    TL0 = TIMER0_RELOAD_L;
    
    s_1ms_tick++;
    s_1ms_flag = 1;
    
    if (Power_GetState() == POWER_STATE_ON)
    {
        Power_IncrementIdleCounter();
    }
}

/**
 * @brief  系统初始化（上电后只执行一次）
 * @note   初始化顺序：
 *         1. 按键模块（P0.0 + INT0）
 *         2. LED模块（P0.2）
 *         3. 马达模块（P0.3）
 *         4. 电源管理（P0.1 + P0.5 + INT4）
 *         5. Timer0（1ms定时中断）
 *         6. 全局中断使能
 */
static void System_Init(void)
{
    Button_Init();
    LED_Init();
    Motor_Init();
    Power_Init();
    Timer0_Init();
    App_Init();
    EA = 1;
}

/**
 * @brief  主函数 - 无限循环（基于1ms精确调度）
 * @note   主循环结构：
 *         ┌────────────────────────────────────────────┐
 *         │ while(1) {                                 │
 *         │   switch(电源状态) {                       │
 *         │     case OFF:                              │
 *         │       App_HandlePowerOff();               │
 *         │       break;                               │
 *         │     case ON:                               │
 *         │       App_HandlePowerOn();                │
 *         │       break;                               │
 *         │   }                                        │
 *         │ }                                           │
 *         └────────────────────────────────────────────┘
 * 
 * @return 永不返回（嵌入式系统主循环）
 */
void main(void)
{
    System_Init();
    
    while (1)
    {
        switch (Power_GetState())
        {
            case POWER_STATE_OFF:
                App_HandlePowerOff();
                break;
                
            case POWER_STATE_ON:
                App_HandlePowerOn();
                break;
                
            default:
                break;
        }
    }
}