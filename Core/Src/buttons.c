#include "buttons.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"

#define DEBOUNCE_TIME_MS  (20)
#define LONG_PRESS_TIME_MS (1500)

void poll_button(Button_t* btn) {
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

bool check_button_event(Button_t* btn, BUTTON_EVENT_E event) {
    if(btn-> event == event) {
        btn -> event = BTN_NO_EVENT;
        return true;
    }
    return false;
}