#include "power.h"
#include "motor.h"

static Power_State_t s_power_state = POWER_STATE_OFF;
static Charger_State_t s_charger_state = CHARGER_STATE_NONE;
static volatile uint32_t s_idle_counter = 0;
static uint8_t s_last_p05_state = 0;

static void Check_Charger_PlugUnplug(void);
static void Update_Charging_Status(void);
static void Check_AutoPowerOff(void);

/**
 * @brief  INT4中断服务程序 - 充电器插入/拔出检测
 * @note   向量号: INTERRUPT_VECTOR_CHARGER (6)
 *         触发条件：P0.5上升沿或下降沿
 */
void Charger_ISR_Handler(void) interrupt INTERRUPT_VECTOR_CHARGER
{
    if (EPIF & 0x04) {
        EPIF = 0x04;
        
        if (GPIO_ReadPin(CHARGER_DETECT_PIN) == GPIO_LEVEL_HIGH) {
            if (GPIO_ReadPin(CHARGE_STATUS_PIN) == GPIO_LEVEL_HIGH) {
                s_charger_state = CHARGER_STATE_FULL;
            } else {
                s_charger_state = CHARGER_STATE_CHARGING;
            }
        } else {
            s_charger_state = CHARGER_STATE_NONE;
        }
    }
}

/**
 * @brief  电源管理模块初始化
 * @note   配置：
 *         - P0.5: 充电器检测（输入，无上下拉）
 *         - P0.1: 充电状态（输入，内部上拉）
 *         - INT4中断：双沿触发（充电器插拔唤醒）
 */
void Power_Init(void)
{
    PWMEN &= ~(1 << 3);
    GPIO_Init(CHARGER_DETECT_PIN, GPIO_MODE_INPUT_ANALOG);
    
    GPIO_Init(CHARGE_STATUS_PIN, GPIO_MODE_INPUT_PULLUP);
    
    EP2CON = (1 << 7) | (1 << 6) | (1 << 5) | (1 << 0);
    INT4EN = 1;
    
    s_power_state = POWER_STATE_OFF;
    s_charger_state = CHARGER_STATE_NONE;
    s_idle_counter = 0;
    s_last_p05_state = GPIO_ReadPin(CHARGER_DETECT_PIN);
}

/**
 * @brief  获取电源状态
 * @return 当前电源状态（ON/OFF）
 */
Power_State_t Power_GetState(void)
{
    return s_power_state;
}

/**
 * @brief  获取充电器状态
 * @return 当前充电器状态（NONE/CHARGING/FULL）
 */
Charger_State_t Power_GetChargerState(void)
{
    return s_charger_state;
}

/**
 * @brief  设置电源状态
 * @param  state: 目标状态（ON/OFF）
 * @note   开机时会重置空闲计时器
 */
void Power_SetState(Power_State_t state)
{
    s_power_state = state;
    
    if (state == POWER_STATE_ON) {
        s_idle_counter = 0;
    }
}

/**
 * @brief  获取空闲计时器值
 * @return 空闲时间（毫秒）
 */
uint32_t Power_GetIdleCounter(void)
{
    return s_idle_counter;
}

/**
 * @brief  重置空闲计时器
 * @note   在有用户操作时调用，防止自动关机
 */
void Power_ResetIdleCounter(void)
{
    s_idle_counter = 0;
}

/**
 * @brief  进入STOP低功耗模式
 * @note   功耗：< 7μA
 *         唤醒源：
 *         - INT0 (P0.0 按键)
 *         - INT4 (P0.5 充电器插拔)
 * @warning 此函数会阻塞直到被唤醒
 */
void Power_EnterStopMode(void)
{
    bit ea_backup;
    uint8_t ckcon_backup;
    
    I2CCON = 0x00;
    MECON |= (1 << 6);
    
    ckcon_backup = CKCON & 0xFE;
    ea_backup = EA;
    EA = 0;
    
    CKCON = 0x00;
    PCON |= 0x02;
    _nop_();
    _nop_();
    _nop_();
    
    CKCON = ckcon_backup | (1 << 7);
    EA = ea_backup;
}

/**
 * @brief  STOP模式唤醒后的恢复处理
 * @note   恢复时钟和中断使能
 */
void Power_WakeUpHandler(void)
{
    CKCON |= (1 << 7);
    EA = 1;
}

/**
 * @brief  检测充电器插入/拔出事件
 * @note   私有函数，由Power_Poll()调用
 *         处理逻辑：
 *         - 插入充电器 → 更新充电状态 + 唤醒系统
 *         - 拔出充电器 → 清除充电状态 + 关闭系统
 */
static void Check_Charger_PlugUnplug(void)
{
    uint8_t current_p05_state;
    uint8_t charge_status;
    
    current_p05_state = GPIO_ReadPin(CHARGER_DETECT_PIN);
    
    if (current_p05_state != s_last_p05_state) {
        current_p05_state = GPIO_ReadPin(CHARGER_DETECT_PIN);
        
        switch (current_p05_state) {
            case GPIO_LEVEL_HIGH:
                charge_status = GPIO_ReadPin(CHARGE_STATUS_PIN);
                
                if (charge_status == GPIO_LEVEL_HIGH) {
                    s_charger_state = CHARGER_STATE_FULL;
                } else {
                    s_charger_state = CHARGER_STATE_CHARGING;
                }
                
                if (s_power_state == POWER_STATE_OFF) {
                    s_power_state = POWER_STATE_ON;
                    s_idle_counter = 0;
                }
                break;
                
            default:
                s_charger_state = CHARGER_STATE_NONE;
                
                if (s_power_state == POWER_STATE_ON) {
                    s_power_state = POWER_STATE_OFF;
                }
                break;
        }
        
        s_last_p05_state = current_p05_state;
    }
}

/**
 * @brief  更新充电状态（充电中/充满）
 * @note   私有函数，由Power_Poll()调用
 *         仅在充电器已连接时才更新
 */
static void Update_Charging_Status(void)
{
    uint8_t charge_status;
    
    if (s_charger_state == CHARGER_STATE_CHARGING || 
        s_charger_state == CHARGER_STATE_FULL) {
        
        charge_status = GPIO_ReadPin(CHARGE_STATUS_PIN);
        
        if (charge_status == GPIO_LEVEL_HIGH) {
            s_charger_state = CHARGER_STATE_FULL;
        } else {
            s_charger_state = CHARGER_STATE_CHARGING;
        }
    }
}

/**
 * @brief  检查是否需要自动关机
 * @note   私有函数，由Power_Poll()调用
 *         条件：
 *         - 系统处于ON状态
 *         - 无充电器连接
 *         - 马达处于IDLE模式
 *         - 空闲时间 >= AUTO_POWEROFF_TIME_MS (5分钟)
 */
static void Check_AutoPowerOff(void)
{
    if (s_power_state != POWER_STATE_ON) {
        return;
    }
    
    if (s_charger_state != CHARGER_STATE_NONE) {
        return;
    }
    
    if (Motor_GetMode() != MOTOR_MODE_IDLE) {
        return;
    }
    
    if (s_idle_counter >= AUTO_POWEROFF_TIME_MS) {
        s_power_state = POWER_STATE_OFF;
    }
}

/**
 * @brief  电源管理轮询函数（非阻塞，需1ms调用一次）
 * @note   实现功能：
 *         1. 检测充电器插拔事件
 *         2. 更新充电状态（充电中/充满）
 *         3. 检查是否满足自动关机条件
 */
void Power_Poll(void)
{
    Check_Charger_PlugUnplug();
    Update_Charging_Status();
    Check_AutoPowerOff();
}