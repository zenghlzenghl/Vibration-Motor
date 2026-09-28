#include "gpio.h"

static u8 code s_gpio_pin_reg[] = {0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6};

void GPIO_Init(GPIO_Pin_t pin, GPIO_Mode_t mode)
{
    u8 reg_addr;
    
    if (pin >= GPIO_PIN_COUNT)
    {
        return;
    }
    
    reg_addr = s_gpio_pin_reg[pin];
    
    switch (mode)
    {
        case GPIO_MODE_INPUT:
            *(u8 xdata *)reg_addr = 0x01;
            break;

        case GPIO_MODE_OUTPUT:
            *(u8 xdata *)reg_addr = 0x00;
            break;

        case GPIO_MODE_INPUT_PULLUP:
            *(u8 xdata *)reg_addr = (1 << 7) | 1;
            break;

        case GPIO_MODE_INPUT_ANALOG:
            *(u8 xdata *)reg_addr = 0x01;
            break;
            
        default:
            break;
    }
}

void GPIO_WritePin(GPIO_Pin_t pin, GPIO_Level_t level)
{
    switch (level)
    {
        case GPIO_LEVEL_HIGH:
            P0 |= (1 << pin);
            break;
            
        default:
            P0 &= ~(1 << pin);
            break;
    }
}

GPIO_Level_t GPIO_ReadPin(GPIO_Pin_t pin)
{
    if ((P0 >> pin) & 0x01)
    {
        return GPIO_LEVEL_HIGH;
    }
    else
    {
        return GPIO_LEVEL_LOW;
    }
}

//void GPIO_TogglePin(GPIO_Pin_t pin)
//{
//    P0 ^= (1 << pin);
//}