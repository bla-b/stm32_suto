#ifndef BUTTONS_H
#define BUTTONS_H


#include "main.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BUTTON_INACTIVE,
    PRESS_BOUNCE,
    BUTTON_ACTIVE,
    BUTTON_LONG_PRESS_REGISTERED,
    RELEASE_BOUNCE
} ButtonState_t;

typedef enum {
    BTN_NO_EVENT,
    BTN_PRESSED,
    BTN_RELEASED,
    BTN_LONG_PRESS
} ButtonEvent_t;

typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    GPIO_PinState activeState;
    ButtonState_t state;
    ButtonEvent_t event;
    uint32_t last_time;
} Button_t;


void button_poll(Button_t* btn);
bool button_check_event(Button_t* btn, ButtonEvent_t event);


//rotary encoder
typedef struct {
    TIM_HandleTypeDef *timerHandle;
    uint32_t encoderZero;
    //uint32_t encoderPos;
} Encoder_t;

void encoder_poll(Encoder_t* enc);
int32_t encoder_get_delta(const Encoder_t* enc);
void encoder_zero(Encoder_t* enc);


#endif