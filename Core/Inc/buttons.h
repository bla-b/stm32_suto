#ifndef BUTTONS_H
#define BUTTONS_H


#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include <stdbool.h>

typedef enum {
    BUTTON_INACTIVE,
    PRESS_BOUNCE,
    BUTTON_ACTIVE,
    BUTTON_LONG_PRESS_REGISTERED,
    RELEASE_BOUNCE
} BUTTON_STATE_E;

typedef enum {
    BTN_NO_EVENT,
    BTN_PRESSED,
    BTN_RELEASED,
    BTN_LONG_PRESS
} BUTTON_EVENT_E;

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    GPIO_PinState activeState;
    BUTTON_STATE_E state;
    BUTTON_EVENT_E event;
    uint32_t last_time;
} Button_t;


void poll_button(Button_t* btn);
bool check_button_event(Button_t* btn, BUTTON_EVENT_E event);

#endif