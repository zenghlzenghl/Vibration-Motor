#include "ca51m550.h"
#include "config.h"
#include "button.h"
#include "led.h"
#include "motor.h"
#include "power.h"

static volatile u16 s_1ms_tick = 0;

static void Timer0_Init(void);
static void System_Init(void);
static void Handle_ButtonEvent(Button_Event_t event);
static void Update_LED_BasedOnCharger(void);

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
 * @note   每1ms执行一次
 *         功能：
 *         - 更新系统1ms计时器
 *         - 更新空闲计时器（用于自动关机判断）
 */
void Timer0_ISR(void) interrupt 1
{
    TH0 = TIMER0_RELOAD_H;
    TL0 = TIMER0_RELOAD_L;
    
    s_1ms_tick++;
    
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
    EA = 1;
}

/**
 * @brief  处理按键事件
 * @param  event: 按键事件类型（长按/短按）
 * @note   事件处理逻辑：
 *         - 长按：开关机切换
 *           → 开机时：设为IDLE模式 + 振动50ms确认 + LED常亮
 *           → 关机时：关闭马达+LED + 进入STOP模式待机
 *         - 短按：切换振动模式（仅在开机状态下有效）
 *           → 循环切换10种模式
 *           → LED闪烁500ms提示
 *           → 重置空闲计时器
 */
static void Handle_ButtonEvent(Button_Event_t event)
{
    static u8 s_startup_vibration_done = 0;
    Motor_Mode_t current_mode;
    Motor_Mode_t next_mode;
    
    if (event == BUTTON_EVENT_NONE)
    {
        return;
    }
    
    switch (event)
    {
        case BUTTON_EVENT_LONG_PRESS:
            switch (Power_GetState())
            {
                case POWER_STATE_ON:
                    Power_SetState(POWER_STATE_OFF);
                    LED_SetState(LED_STATE_OFF);
                    Motor_Control(0);
                    Motor_SetMode(MOTOR_MODE_IDLE);
                    s_startup_vibration_done = 0;
                    break;
                    
                default:
                    Power_SetState(POWER_STATE_ON);
                    Motor_SetMode(MOTOR_MODE_IDLE);
                    LED_SetState(LED_STATE_ON);
                    
                    Motor_Control(1);
                    s_1ms_tick = 0;
                    while (s_1ms_tick < STARTUP_VIBRATION_MS)
                    {
                        ;
                    }
                    Motor_Control(0);
                    
                    s_startup_vibration_done = 1;
                    break;
            }
            break;
            
        case BUTTON_EVENT_SHORT_PRESS:
            if (Power_GetState() == POWER_STATE_ON)
            {
                current_mode = Motor_GetMode();
                next_mode = (Motor_Mode_t)((current_mode + 1) % MOTOR_MODE_COUNT);
                Motor_SetMode(next_mode);
                LED_StartBlink500ms();
                Power_ResetIdleCounter();
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief  根据充电状态更新LED显示
 * @note   仅在开机状态下执行
 *         优先级：
 *         1. BLINK_500MS状态（最高优先级，不可覆盖）
 *         2. 充电中→500ms闪烁
 *         3. 充满电→常亮
 *         4. 无充电器 → 恢复之前状态
 */
static void Update_LED_BasedOnCharger(void)
{
    Charger_State_t charger;
    LED_State_t led_state;
    
    if (Power_GetState() != POWER_STATE_ON)
    {
        return;
    }
    
    charger = Power_GetChargerState();
    led_state = LED_GetState();
    
    if (led_state == LED_STATE_BLINK_500MS)
    {
        return;
    }
    
    switch (charger)
    {
        case CHARGER_STATE_CHARGING:
            LED_SetState(LED_STATE_BLINK_CHARGING);
            break;
            
        case CHARGER_STATE_FULL:
            LED_SetState(LED_STATE_ON);
            break;
            
        default:
            if (led_state == LED_STATE_BLINK_CHARGING)
            {
                LED_SetState(LED_STATE_ON);
            }
            break;
    }
}

/**
 * @brief  主函数 - 无限循环
 * @note   主循环结构：
 *         ┌────────────────────────────────────────────┐
 *         │ while(1) {                                 │
 *         │   if (POWER_OFF) {                         │
 *         │     EnterStopMode(); // 低功耗等待唤醒      │
 *         │     处理唤醒事件;                          │
 *         │   } else {                                 │
 *         │     Button_Poll();   // 1ms轮询             │
 *         │     Power_Poll();    // 1ms轮询             │
 *         │     Motor_Poll();    // 1ms轮询             │
 *         │     LED_Poll();      // 1ms轮询             │
 *         │     UpdateLED();     // 充电状态指示更新     │
 *         │   }                                         │
 *         │ }                                           │
 *         └────────────────────────────────────────────┘
 * 
 * @return 永不返回（嵌入式系统主循环）
 */
void main(void)
{
    Button_Event_t button_event;
    
    System_Init();
    
    while (1)
    {
        switch (Power_GetState())
        {
            case POWER_STATE_OFF:
                Power_EnterStopMode();
                Power_WakeUpHandler();
                
                if (Button_GetEvent() != BUTTON_EVENT_NONE || 
                    Power_GetChargerState() != CHARGER_STATE_NONE)
                {
                    Handle_ButtonEvent(BUTTON_EVENT_LONG_PRESS);
                    Button_ClearEvent();
                }
                break;
                
            case POWER_STATE_ON:
                Button_Poll();
                button_event = Button_GetEvent();
                
                if (button_event != BUTTON_EVENT_NONE)
                {
                    Handle_ButtonEvent(button_event);
                    Button_ClearEvent();
                }
                
                Power_Poll();
                
                if (Power_GetState() == POWER_STATE_ON)
                {
                    Motor_Poll();
                    LED_Poll();
                    Update_LED_BasedOnCharger();
                    
                    if (Power_GetState() == POWER_STATE_OFF)
                    {
                        LED_SetState(LED_STATE_OFF);
                        Motor_Control(0);
                        Motor_SetMode(MOTOR_MODE_IDLE);
                    }
                }
                break;
                
            default:
                break;
        }
    }
}