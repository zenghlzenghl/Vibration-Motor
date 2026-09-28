#ifndef __LED_H__
#define __LED_H__

#include "gpio.h"
#include "config.h"
//#include <stdint.h>

typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON,
    LED_STATE_BLINK_CHARGING,
    LED_STATE_BLINK_500MS
} LED_State_t;

void LED_Init(void);
void LED_SetState(LED_State_t state);
void LED_Poll(void);
LED_State_t LED_GetState(void);
void LED_StartBlink500ms(void);

#endif
