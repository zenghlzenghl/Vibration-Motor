#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "gpio.h"
#include "config.h"
//#include <stdint.h>

typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS
} Button_Event_t;

void Button_Init(void);
void Button_Poll(void);
Button_Event_t Button_GetEvent(void);
void Button_ClearEvent(void);
//u8 Button_IsPressed(void);

#endif
