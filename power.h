#ifndef __POWER_H__
#define __POWER_H__

#include "gpio.h"
#include "config.h"
#include <stdint.h>

typedef enum {
    POWER_STATE_OFF = 0,
    POWER_STATE_ON
} Power_State_t;

typedef enum {
    CHARGER_STATE_NONE = 0,
    CHARGER_STATE_CHARGING,
    CHARGER_STATE_FULL
} Charger_State_t;

void Power_Init(void);
Power_State_t Power_GetState(void);
Charger_State_t Power_GetChargerState(void);
void Power_SetState(Power_State_t state);
uint32_t Power_GetIdleCounter(void);
void Power_ResetIdleCounter(void);
void Power_EnterStopMode(void);
void Power_WakeUpHandler(void);
void Power_Poll(void);

#endif