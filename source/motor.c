#include "motor.h"

static Motor_Mode_t s_motor_mode = MOTOR_MODE_IDLE;
static u8 s_motor_running = 0;
static u16 s_pattern_timer = 0;
static u8 s_pattern_step = 0;
static u8 s_ramp_duty = 0;
static u8 s_random_seed = 123;

static u8 Get_Random_Byte(void);
void Motor_Mode_Idle(void);
void Motor_Mode_Continuous(void);
void Motor_Mode_Interval_1S(void);
void Motor_Mode_Interval_2S(void);
void Motor_Mode_Pattern_1(void);
void Motor_Mode_Pattern_2(void);
void Motor_Mode_Pattern_3(void);
void Motor_Mode_Ramp_Up(void);
void Motor_Mode_Ramp_Down(void);
void Motor_Mode_Random(void);

/**
 * @brief  伪随机数生成器（线性同余算法）
 * @return 0-255范围内的随机字节
 * @note   使用静态种子，每次调用更新种子
 */
static u8 Get_Random_Byte(void)
{
    s_random_seed = s_random_seed * 1103515245 + 12345;
    return (u8)(s_random_seed >> 16);
}

/**
 * @brief  马达模块初始化
 * @note   配置P0.3为推挽输出，初始状态为关闭
 */
void Motor_Init(void)
{
    GPIO_Init(MOTOR_PIN, GPIO_MODE_OUTPUT);
    GPIO_WritePin(MOTOR_PIN, GPIO_LEVEL_LOW);
    
    s_motor_mode = MOTOR_MODE_IDLE;
    s_motor_running = 0;
    s_pattern_timer = 0;
    s_pattern_step = 0;
    s_ramp_duty = 0;
}

/**
 * @brief  直接控制马达开关
 * @param  enable: 1=开启马达, 0=关闭马达
 */
void Motor_Control(u8 enable)
{
    if (enable)
    {
        GPIO_WritePin(MOTOR_PIN, GPIO_LEVEL_HIGH);
        s_motor_running = 1;
    }
    else
    {
        GPIO_WritePin(MOTOR_PIN, GPIO_LEVEL_LOW);
        s_motor_running = 0;
    }
}

/**
 * @brief  设置振动模式
 * @param  mode: 目标模式（参考Motor_Mode_t枚举）
 * @note   切换模式时会重置所有内部状态（计时器、步进、占空比）
 */
void Motor_SetMode(Motor_Mode_t mode)
{
    if (mode >= MOTOR_MODE_COUNT)
    {
        mode = MOTOR_MODE_IDLE;
    }
    
    s_motor_mode = mode;
    s_pattern_step = 0;
    s_pattern_timer = 0;
    s_ramp_duty = 0;
    
    if (mode == MOTOR_MODE_IDLE)
    {
        Motor_Control(0);
    }
}

/**
 * @brief  获取当前振动模式
 * @return 当前模式
 */
Motor_Mode_t Motor_GetMode(void)
{
    return s_motor_mode;
}

/**
 * @brief  模式0：空闲模式
 * @note   始终保持关闭状态
 */
void Motor_Mode_Idle(void)
{
    if (s_motor_running == 1)
    {
        Motor_Control(0);
    }
}

/**
 * @brief  模式1：连续模式
 * @note   马达持续振动直到切换到其他模式
 */
void Motor_Mode_Continuous(void)
{
    if (s_motor_running == 0)
    {
        Motor_Control(1);
    }
}

/**
 * @brief  模式2：间隔1秒模式
 * @note   时序：500ms振动 + 500ms停止（循环）
 */
void Motor_Mode_Interval_1S(void)
{
    if (s_pattern_timer == 0)
    {
        switch (s_pattern_step)
        {
            case 0:
                Motor_Control(1);
                s_pattern_timer = 500;
                s_pattern_step = 1;
                break;
                
            default:
                Motor_Control(0);
                s_pattern_timer = 500;
                s_pattern_step = 0;
                break;
        }
    }
    else
    {
        s_pattern_timer--;
    }
}

/**
 * @brief  模式3：间隔2秒模式
 * @note   时序：500ms振动 + 1500ms停止（循环）
 */
void Motor_Mode_Interval_2S(void)
{
    if (s_pattern_timer == 0)
    {
        switch (s_pattern_step)
        {
            case 0:
                Motor_Control(1);
                s_pattern_timer = 500;
                s_pattern_step = 1;
                break;
                
            default:
                Motor_Control(0);
                s_pattern_timer = 1500;
                s_pattern_step = 0;
                break;
        }
    }
    else
    {
        s_pattern_timer--;
    }
}

/**
 * @brief  模式4：节奏模式1（短-短-长停）
 * @note   时序：200ms振 - 100ms停 - 200ms振 - 500ms停
 *         适合发送短消息通知的节奏
 */
void Motor_Mode_Pattern_1(void)
{
    if (s_pattern_timer == 0)
    {
        switch (s_pattern_step)
        {
            case 0:
                Motor_Control(1);
                s_pattern_timer = MOTOR_VIBRATE_200MS;
                break;
                
            case 1:
                Motor_Control(0);
                s_pattern_timer = MOTOR_STOP_100MS;
                break;
                
            case 2:
                Motor_Control(1);
                s_pattern_timer = MOTOR_VIBRATE_200MS;
                break;
                
            case 3:
                Motor_Control(0);
                s_pattern_timer = MOTOR_STOP_500MS;
                break;
                
            default:
                break;
        }
        s_pattern_step = (s_pattern_step + 1) % 4;
    }
    else
    {
        s_pattern_timer--;
    }
}

/**
 * @brief  模式5：节奏模式2（三连击+长停）
 * @note   时序：100ms振 - 100ms停 × 3次 - 700ms长停
 *         适合紧急提醒场景
 */
void Motor_Mode_Pattern_2(void)
{
    if (s_pattern_timer == 0)
    {
        switch (s_pattern_step)
        {
            case 0:
                Motor_Control(1);
                s_pattern_timer = MOTOR_VIBRATE_100MS;
                break;
                
            case 1:
                Motor_Control(0);
                s_pattern_timer = MOTOR_STOP_100MS;
                break;
                
            case 2:
                Motor_Control(1);
                s_pattern_timer = MOTOR_VIBRATE_100MS;
                break;
                
            case 3:
                Motor_Control(0);
                s_pattern_timer = MOTOR_STOP_100MS;
                break;
                
            case 4:
                Motor_Control(1);
                s_pattern_timer = MOTOR_VIBRATE_100MS;
                break;
                
            case 5:
                Motor_Control(0);
                s_pattern_timer = MOTOR_STOP_700MS;
                break;
                
            default:
                break;
        }
        s_pattern_step = (s_pattern_step + 1) % 6;
    }
    else
    {
        s_pattern_timer--;
    }
}

/**
 * @brief  模式6：节奏模式3（长短组合）
 * @note   时序：300ms振 - 200ms停 - 100ms振 - 400ms停
 *              - 150ms振 - 650ms停
 *         适合复杂节奏的音乐感振动
 */
void Motor_Mode_Pattern_3(void)
{
    if (s_pattern_timer == 0)
    {
        switch (s_pattern_step)
        {
            case 0:
                Motor_Control(1);
                s_pattern_timer = MOTOR_VIBRATE_300MS;
                break;
                
            case 1:
                Motor_Control(0);
                s_pattern_timer = MOTOR_STOP_200MS;
                break;
                
            case 2:
                Motor_Control(1);
                s_pattern_timer = MOTOR_VIBRATE_100MS;
                break;
                
            case 3:
                Motor_Control(0);
                s_pattern_timer = MOTOR_STOP_400MS;
                break;
                
            case 4:
                Motor_Control(1);
                s_pattern_timer = MOTOR_VIBRATE_150MS;
                break;
                
            case 5:
                Motor_Control(0);
                s_pattern_timer = MOTOR_STOP_650MS;
                break;
                
            default:
                break;
        }
        s_pattern_step = (s_pattern_step + 1) % 6;
    }
    else
    {
        s_pattern_timer--;
    }
}

/**
 * @brief  模式7：渐强模式
 * @note   占空比从0%逐渐增加到100%，每50ms增加2%
 *         达到100%后重置为0%（循环）
 *         模拟"呼吸灯"效果的振动感觉
 */
void Motor_Mode_Ramp_Up(void)
{
    if (s_pattern_timer == 0)
    {
        if (s_ramp_duty < RAMP_MAX_DUTY)
        {
            s_ramp_duty += RAMP_STEP;
            if (s_ramp_duty > RAMP_MAX_DUTY)
            {
                s_ramp_duty = RAMP_MAX_DUTY;
            }
        }
        
        Motor_Control(1);
        s_pattern_timer = RAMP_UPDATE_INTERVAL_MS;
        
        if (s_ramp_duty >= RAMP_MAX_DUTY && s_pattern_timer <= 1)
        {
            s_ramp_duty = 0;
        }
    }
    else
    {
        s_pattern_timer--;
    }
}

/**
 * @brief  模式8：渐弱模式
 * @note   占空比从100%逐步减少到0%，每50ms减少2%
 *         达到0%后重置为100%（循环）
 *         与RAMP_UP形成对称效果
 */
void Motor_Mode_Ramp_Down(void)
{
    if (s_pattern_timer == 0)
    {
        if (s_ramp_duty > 0)
        {
            s_ramp_duty -= RAMP_STEP;
            if (s_ramp_duty > RAMP_MAX_DUTY)
            {
                s_ramp_duty = 0;
            }
        }
        
        Motor_Control(s_ramp_duty > 0 ? 1 : 0);
        s_pattern_timer = RAMP_UPDATE_INTERVAL_MS;
        
        if (s_ramp_duty == 0 && s_pattern_timer <= 1)
        {
            s_ramp_duty = RAMP_MAX_DUTY;
        }
    }
    else
    {
        s_pattern_timer--;
    }
}

/**
 * @brief  模式9：随机模式
 * @note   使用伪随机数生成不可预测的时序
 *         - 振动时间：RANDOM_VIBRATE_MIN_MS ~ RANDOM_VIBRATE_MAX_MS
 *         - 停止时间：RANDOM_STOP_MIN_MS ~ RANDOM_STOP_MAX_MS
 *         适合游戏等需要趣味性的场景
 */
void Motor_Mode_Random(void)
{
    u8 random_val;
    
    if (s_pattern_timer == 0)
    {
        random_val = Get_Random_Byte();
        
        if (random_val < 128)
        {
            Motor_Control(1);
            s_pattern_timer = RANDOM_VIBRATE_MIN_MS + (random_val % 
                            (RANDOM_VIBRATE_MAX_MS - RANDOM_VIBRATE_MIN_MS + 1));
        }
        else
        {
            Motor_Control(0);
            s_pattern_timer = RANDOM_STOP_MIN_MS + ((random_val - 128) % 
                            (RANDOM_STOP_MAX_MS - RANDOM_STOP_MIN_MS + 1));
        }
    }
    else
    {
        s_pattern_timer--;
    }
}

/**
 * @brief  马达轮询函数（非阻塞，需1ms调用一次）
 * @note   根据当前模式调用对应的模式处理函数
 *         这是唯一的对外调度接口
 */
void Motor_Poll(void)
{
    switch (s_motor_mode)
    {
        case MOTOR_MODE_IDLE:
            Motor_Mode_Idle();
            break;
            
        case MOTOR_MODE_CONTINUOUS:
            Motor_Mode_Continuous();
            break;
            
        case MOTOR_MODE_INTERVAL_1S:
            Motor_Mode_Interval_1S();
            break;
            
        case MOTOR_MODE_INTERVAL_2S:
            Motor_Mode_Interval_2S();
            break;
            
        case MOTOR_MODE_PATTERN_1:
            Motor_Mode_Pattern_1();
            break;
            
        case MOTOR_MODE_PATTERN_2:
            Motor_Mode_Pattern_2();
            break;
            
        case MOTOR_MODE_PATTERN_3:
            Motor_Mode_Pattern_3();
            break;
            
        case MOTOR_MODE_RAMP_UP:
            Motor_Mode_Ramp_Up();
            break;
            
        case MOTOR_MODE_RAMP_DOWN:
            Motor_Mode_Ramp_Down();
            break;
            
        case MOTOR_MODE_RANDOM:
            Motor_Mode_Random();
            break;
            
        default:
            if (s_motor_running == 1)
            {
                Motor_Control(0);
            }
            break;
    }
}