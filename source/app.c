#include "ca51m550.h"
#include "config.h"
#include "app.h"
#include "button.h"
#include "led.h"
#include "motor.h"
#include "power.h"

static u8 s_startup_vibration_done = 0;

extern volatile u16 s_1ms_tick;
extern volatile bit s_1ms_flag;

static void Handle_ButtonEvent(Button_Event_t event);
static void Update_LED_BasedOnCharger(void);

/**
 * @brief  应用层初始化
 */
void App_Init(void)
{
    s_startup_vibration_done = 0;
}

/**
 * @brief  处理关机状态
 * @note   在STOP模式唤醒后调用
 */
void App_HandlePowerOff(void)
{
    Button_Event_t button_event;
    
    Power_EnterStopMode();
    Power_WakeUpHandler();
    
    if (Power_GetChargerState() != CHARGER_STATE_NONE)
    {
        Handle_ButtonEvent(BUTTON_EVENT_LONG_PRESS);
        Button_ClearEvent();
        Button_ClearWakeFlag();
    }
    else if (Button_WokenFromStop())
    {
        Button_ClearWakeFlag();
        
        while (Power_GetState() == POWER_STATE_OFF)
        {
            if (s_1ms_flag)
            {
                s_1ms_flag = 0;
                
                Button_Poll();
                button_event = Button_GetEvent();
                
                if (button_event != BUTTON_EVENT_NONE)
                {
                    if (button_event == BUTTON_EVENT_LONG_PRESS)
                    {
                        Handle_ButtonEvent(button_event);
                    }
                    Button_ClearEvent();
                    break;
                }
            }
        }
    }
}

/**
 * @brief  处理开机状态
 * @note   每1ms调用一次
 */
void App_HandlePowerOn(void)
{
    Button_Event_t button_event;
    
    if (s_1ms_flag)
    {
        s_1ms_flag = 0;
        
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
    }
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