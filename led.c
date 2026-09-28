#include "led.h"

static LED_State_t s_led_state = LED_STATE_OFF;
static u16 s_blink_counter = 0;
static u16 s_blink_timer = 0;
static u8 s_led_current_output = 0;

/**
 * @brief  LED模块初始化
 * @note   配置P0.2为推挽输出，初始状态为关闭
 */
void LED_Init(void)
{
    GPIO_Init(LED_PIN, GPIO_MODE_OUTPUT);
    GPIO_WritePin(LED_PIN, GPIO_LEVEL_LOW);
    
    s_led_state = LED_STATE_OFF;
    s_blink_counter = 0;
    s_blink_timer = 0;
    s_led_current_output = 0;
}

/**
 * @brief  设置LED状态
 * @param  state: 目标状态（OFF/ON/BLINK_CHARGING/BLINK_500MS）
 * @note   调用此函数会重置闪烁计时器
 */
void LED_SetState(LED_State_t state)
{
    s_led_state = state;
    s_blink_counter = 0;
    s_blink_timer = 0;
    
    switch (state)
    {
        case LED_STATE_OFF:
            GPIO_WritePin(LED_PIN, GPIO_LEVEL_LOW);
            s_led_current_output = 0;
            break;
            
        case LED_STATE_ON:
            GPIO_WritePin(LED_PIN, GPIO_LEVEL_HIGH);
            s_led_current_output = 1;
            break;
            
        default:
            break;
    }
}

/**
 * @brief  LED轮询函数（非阻塞，需1ms调用一次）
 * @note   实现功能：
 *         - OFF: 关闭LED
 *         - ON: 点亮LED
 *         - BLINK_CHARGING: 500ms周期闪烁（充电指示）
 *         - BLINK_500MS: 熄灭500ms后恢复（模式切换提示）
 */
void LED_Poll(void)
{
    switch (s_led_state)
    {
        case LED_STATE_OFF:
            if (s_led_current_output == 1)
            {
                GPIO_WritePin(LED_PIN, GPIO_LEVEL_LOW);
                s_led_current_output = 0;
            }
            break;
            
        case LED_STATE_ON:
            if (s_led_current_output == 0)
            {
                GPIO_WritePin(LED_PIN, GPIO_LEVEL_HIGH);
                s_led_current_output = 1;
            }
            break;
            
        case LED_STATE_BLINK_CHARGING:
            s_blink_counter++;
            if (s_blink_counter >= LED_BLINK_CHARGING_PERIOD)
            {
                s_blink_counter = 0;
                s_led_current_output = !s_led_current_output;
                GPIO_WritePin(LED_PIN, s_led_current_output ? 
                              GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
            }
            break;
            
        case LED_STATE_BLINK_500MS:
            if (s_blink_timer > 0)
            {
                s_blink_timer--;
                if (s_led_current_output == 1)
                {
                    GPIO_WritePin(LED_PIN, GPIO_LEVEL_LOW);
                    s_led_current_output = 0;
                }
            }
            else
            {
                GPIO_WritePin(LED_PIN, GPIO_LEVEL_HIGH);
                s_led_current_output = 1;
                s_led_state = LED_STATE_ON;
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief  获取LED当前状态
 * @return 当前LED状态
 */
LED_State_t LED_GetState(void)
{
    return s_led_state;
}

/**
 * @brief  启动500ms熄灭模式
 * @note   用于短按切换模式时的视觉反馈
 */
void LED_StartBlink500ms(void)
{
    s_led_state = LED_STATE_BLINK_500MS;
    s_blink_timer = LED_BLINK_500MS_DURATION;
    s_blink_counter = 0;
}