#include "button.h"

static u8 s_button_state = 0;
static u16 s_press_counter = 0;
static u8 s_debounce_counter = 0;
static u8 s_last_raw_state = 1;
static volatile Button_Event_t s_button_event = BUTTON_EVENT_NONE;
static volatile bit s_wake_from_stop = 0;

/**
 * @brief  按键模块初始化
 * @note   配置P0.0为输入模式（内部上拉）
 *         使能INT0中断（下降沿触发）- 仅用于STOP模式唤醒
 *         按键事件检测由Button_Poll()负责（1ms调用一次）
 */
void Button_Init(void)
{
    GPIO_Init(BUTTON_PIN, GPIO_MODE_INPUT_PULLUP);
    IT0 = 1;
    EX0 = 1;
    
    s_button_state = 0;
    s_press_counter = 0;
    s_debounce_counter = 0;
    s_last_raw_state = 1;
    s_button_event = BUTTON_EVENT_NONE;
    s_wake_from_stop = 0;
}

/**
 * @brief  INT0中断服务程序 - STOP模式唤醒
 * @note   向量号: INTERRUPT_VECTOR_BUTTON (0)
 *         功能：仅用于从STOP低功耗模式唤醒CPU
 *         设置唤醒标志，供主循环检测唤醒源
 *         
 *         不设置按键事件，避免与Button_Poll()冲突
 *         按键事件检测完全由Button_Poll()负责：
 *         - 20ms软件消抖
 *         - 长短按区分（1.5秒阈值）
 *         - 状态机：释放→按下→事件→释放
 */
void Button_ISR(void) interrupt INTERRUPT_VECTOR_BUTTON
{
    s_wake_from_stop = 1;
}

/**
 * @brief  获取按键事件
 * @return 当前按键事件类型
 */
Button_Event_t Button_GetEvent(void)
{
    return s_button_event;
}

/**
 * @brief  清除按键事件
 */
void Button_ClearEvent(void)
{
    s_button_event = BUTTON_EVENT_NONE;
}

/**
 * @brief  检测是否从STOP模式被按键唤醒
 * @return 1=按键唤醒, 0=其他原因唤醒
 * @note   在STOP模式唤醒后调用，用于区分唤醒源
 */
bit Button_WokenFromStop(void)
{
    return s_wake_from_stop;
}

/**
 * @brief  清除STOP模式唤醒标志
 */
void Button_ClearWakeFlag(void)
{
    s_wake_from_stop = 0;
}

/**
 * @brief  查询按键当前是否被按下
 * @return 1=按下, 0=释放
 */
//u8 Button_IsPressed(void)
//{
//    return (GPIO_ReadPin(BUTTON_PIN) == GPIO_LEVEL_LOW);
//}

/**
 * @brief  按键轮询函数（非阻塞，需1ms调用一次）
 * @note   实现功能：
 *         - 20ms软件消抖
 *         - 长按/短按判定（阈值1.5秒）
 *         - 状态机：释放→按下→事件→释放
 */
void Button_Poll(void)
{
    u8 current_raw_state;
    
    current_raw_state = GPIO_ReadPin(BUTTON_PIN);
    
    if (s_debounce_counter < BUTTON_DEBOUNCE_MS)
    {
        if (current_raw_state != s_last_raw_state)
        {
            s_debounce_counter = 0;
            s_last_raw_state = current_raw_state;
        }
        else
        {
            s_debounce_counter++;
        }
    }
    
    if (s_debounce_counter >= BUTTON_DEBOUNCE_MS)
    {
        switch (current_raw_state)
        {
            case GPIO_LEVEL_LOW:
                if (s_button_state == 0)
                {
                    s_button_state = 1;
                    s_press_counter = 0;
                }
                else
                {
                    if (s_press_counter < 0xFFFF)
                    {
                        s_press_counter++;
                    }
                }
                break;
                
            default:
                if (s_button_state == 1)
                {
                    if (s_press_counter >= BUTTON_LONG_PRESS_MS)
                    {
                        s_button_event = BUTTON_EVENT_LONG_PRESS;
                    }
                    else
                    {
                        s_button_event = BUTTON_EVENT_SHORT_PRESS;
                    }
                    s_button_state = 0;
                    s_press_counter = 0;
                }
                break;
        }
    }
}