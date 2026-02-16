#include "buttons.h"

#define DEBOUNCE_TIME_MS  (50)
#define LONG_PRESS_TIME_MS (1500)

#define ENCODER_PRESCALER (-2) //a timer 2-et szamol kattanasonkent

void button_poll(Button_t* btn) {
    bool currentlyPressed = (btn -> activeState == HAL_GPIO_ReadPin(btn->port, btn->pin));

    switch(btn->state) {
        case BUTTON_INACTIVE:
            if(currentlyPressed) {
                btn-> last_time = HAL_GetTick();
                btn-> state = PRESS_BOUNCE;
                btn-> event = BTN_PRESSED; //azonnal megjelenik az event
            }
            break;
        case PRESS_BOUNCE:
            //20ms cooldown
            if(HAL_GetTick() - btn-> last_time > DEBOUNCE_TIME_MS) {
                btn-> last_time = HAL_GetTick();
                btn-> state = BUTTON_ACTIVE;
            }
        break;
        case BUTTON_ACTIVE:
            if(!currentlyPressed) {
                btn-> last_time = HAL_GetTick();
                btn-> state = RELEASE_BOUNCE;
                btn-> event = BTN_RELEASED;
            }
            //Long press:
            if(HAL_GetTick() - btn-> last_time > LONG_PRESS_TIME_MS) {
                btn-> event = BTN_LONG_PRESS;
                btn-> state = BUTTON_LONG_PRESS_REGISTERED; 
            }
            break;
        case BUTTON_LONG_PRESS_REGISTERED:
            if(!currentlyPressed) {
                btn-> last_time = HAL_GetTick();
                btn-> state = RELEASE_BOUNCE; //nincs event
            }
            break;
        case RELEASE_BOUNCE:
            //20ms cooldown
                if((HAL_GetTick() - btn-> last_time) > DEBOUNCE_TIME_MS) {
                    //btn-> last_time = HAL_GetTick();
                    btn-> state = BUTTON_INACTIVE;
                }
            break;
        default:
            Error_Handler();
            break;
    }

}

bool button_check_event(Button_t* btn, ButtonEvent_t event) {
    if(btn-> event == event) {
        btn -> event = BTN_NO_EVENT;
        return true;
    }
    return false;
}

//encoder 
/*
void encoder_poll(Encoder_t* enc) {
    enc->encoderPos = __HAL_TIM_GET_COUNTER(enc->timerHandle);
}
*/

int32_t encoder_get_delta(const Encoder_t* enc) {
    return ((int16_t)(__HAL_TIM_GET_COUNTER(enc->timerHandle) - enc->encoderZero)) / ENCODER_PRESCALER;
}

void encoder_zero(Encoder_t* enc) {
    enc->encoderZero = __HAL_TIM_GET_COUNTER(enc->timerHandle);
}