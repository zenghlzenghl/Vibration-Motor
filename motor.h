#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "gpio.h"
#include "config.h"
//#include <stdint.h>

typedef enum {
    MOTOR_MODE_IDLE = 0,
    MOTOR_MODE_CONTINUOUS,
    MOTOR_MODE_INTERVAL_1S,
    MOTOR_MODE_INTERVAL_2S,
    MOTOR_MODE_PATTERN_1,
    MOTOR_MODE_PATTERN_2,
    MOTOR_MODE_PATTERN_3,
    MOTOR_MODE_RAMP_UP,
    MOTOR_MODE_RAMP_DOWN,
    MOTOR_MODE_RANDOM,
    MOTOR_MODE_COUNT
} Motor_Mode_t;

void Motor_Init(void);
void Motor_SetMode(Motor_Mode_t mode);
Motor_Mode_t Motor_GetMode(void);
void Motor_Poll(void);
void Motor_Control(u8 enable);

#endif
