#include "button.h"

static uint8_t s_button_state = 0;
static uint16_t s_press_counter = 0;
static uint8_t s_debounce_counter = 0;
static uint8_t s_last_raw_state = 1;
static volatile Button_Event_t s_button_event = BUTTON_EVENT_NONE;

/**
 * @brief  按键模块初始化
 * @note   配置P0.0为输入模式+内部上拉
 *         使能INT0中断（下降沿触发）
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
}

/**
 * @brief  INT0中断服务程序 - 按键唤醒
 * @note   向量号: INTERRUPT_VECTOR_BUTTON (0)
 */
void Button_ISR(void) interrupt INTERRUPT_VECTOR_BUTTON
{
    s_button_event = BUTTON_EVENT_SHORT_PRESS;
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
 * @brief  查询按键当前是否被按下
 * @return 1=按下, 0=释放
 */
uint8_t Button_IsPressed(void)
{
    return (GPIO_ReadPin(BUTTON_PIN) == GPIO_LEVEL_LOW);
}

/**
 * @brief  按键轮询函数（非阻塞，需1ms调用一次）
 * @note   实现功能：
 *         - 20ms软件消抖
 *         - 长按/短按检测（阈值1.5秒）
 *         - 状态机：释放→按下计时→释放判断
 */
void Button_Poll(void)
{
    uint8_t current_raw_state;
    
    current_raw_state = GPIO_ReadPin(BUTTON_PIN);
    
    if (s_debounce_counter < BUTTON_DEBOUNCE_MS) {
        if (current_raw_state != s_last_raw_state) {
            s_debounce_counter = 0;
            s_last_raw_state = current_raw_state;
        } else {
            s_debounce_counter++;
        }
    }
    
    if (s_debounce_counter >= BUTTON_DEBOUNCE_MS) {
        switch (current_raw_state) {
            case GPIO_LEVEL_LOW:
                if (s_button_state == 0) {
                    s_button_state = 1;
                    s_press_counter = 0;
                } else {
                    if (s_press_counter < 0xFFFF) {
                        s_press_counter++;
                    }
                }
                break;
                
            default:
                if (s_button_state == 1) {
                    if (s_press_counter >= BUTTON_LONG_PRESS_MS) {
                        s_button_event = BUTTON_EVENT_LONG_PRESS;
                    } else {
                        s_button_event = BUTTON_EVENT_SHORT_PRESS;
                    }
                    s_button_state = 0;
                    s_press_counter = 0;
                }
                break;
        }
    }
}