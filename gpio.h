#ifndef __GPIO_H__
#define __GPIO_H__

#include "ca51m550.h"
#include <stdint.h>

#define GPIO_PIN_0               0
#define GPIO_PIN_1               1
#define GPIO_PIN_2               2
#define GPIO_PIN_3               3
#define GPIO_PIN_4               4
#define GPIO_PIN_5               5
#define GPIO_PIN_COUNT           6

typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT_PULLUP,
    GPIO_MODE_INPUT_ANALOG
} GPIO_Mode_t;

typedef enum {
    GPIO_LEVEL_LOW = 0,
    GPIO_LEVEL_HIGH
} GPIO_Level_t;

void GPIO_Init(GPIO_Pin_t pin, GPIO_Mode_t mode);
void GPIO_WritePin(GPIO_Pin_t pin, GPIO_Level_t level);
GPIO_Level_t GPIO_ReadPin(GPIO_Pin_t pin);
void GPIO_TogglePin(GPIO_Pin_t pin);

#endif