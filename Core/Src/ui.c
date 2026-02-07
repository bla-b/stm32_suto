#include "ui.h"
#include "buttons.h"
#include "main.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define MENU_OPTIONS_NUM (2)

static Setting_t setTemp = {
    .name = "",
    .value = 81.5,
    .digits = 4,
    .decimalPlaces = 1,
    .printfStr = "Set: %5.1f*C",
    .firstDigitOffset = 5
};

static Setting_t pid_settings[3] = {
    {
        .name = "P",
        .value = 0.1,
        .digits = 4,
        .decimalPlaces = 3,
        .printfStr = "P: %5.3f",
        .firstDigitOffset = 3
    },
    {
        .name = "I",
        .value = 0.0,
        .digits = 4,
        .decimalPlaces = 3,
        .printfStr = "I: %5.3f",
        .firstDigitOffset = 3
    },
    {
        .name = "D",
        .value = 0.0,
        .digits = 4,
        .decimalPlaces = 3,
        .printfStr = "D: %5.3f",
        .firstDigitOffset = 3
    }
};

static int ten_to_the_pow(int exponent) {
    int num = 1;
    for(int i = 0; i < exponent; i++) {
        num *= 10;
    }
    return num;
}

static void setter_begin(SetterState_t *st, Setting_t *setting, Encoder_t *encoder)
{
    st->setting = setting;
    st->currentValue = setting->value;
    st->currentDigitNum = setting->digits - 1;
    st->intValue = (int)(setting->value * ten_to_the_pow(setting->decimalPlaces));

    st->currentDigitVal = st->intValue / ten_to_the_pow(st->currentDigitNum);
    st->intValue -= st->currentDigitVal * ten_to_the_pow(st->currentDigitNum);


    encoder_zero(encoder);
}

static bool setter_poll(SetterState_t *st, Encoder_t *encoder, Button_t* btn) {
    if(st->currentDigitNum >= 0) {
        int digit = st->currentDigitVal + encoder_get_delta(encoder);
        if(digit > 9) {
            st->currentDigitVal = digit = 9;
            encoder_zero(encoder);
        }
        else if(digit < 0) {
            st->currentDigitVal = digit = 0;
            encoder_zero(encoder);
        }

        st->currentValue = (st->intValue + digit * ten_to_the_pow(st->currentDigitNum))/((double)ten_to_the_pow(st->setting->decimalPlaces));

        if(button_check_event(btn, BTN_RELEASED)) {
            st->intValue = st->intValue + digit * ten_to_the_pow(st->currentDigitNum);
            st->currentDigitNum--;

            if(st->currentDigitNum >= 0) {
                st->currentDigitVal = (st->intValue / ten_to_the_pow(st->currentDigitNum)) % 10;
                st->intValue -= st->currentDigitVal * ten_to_the_pow(st->currentDigitNum);
            }

            encoder_zero(encoder);
        }
    }
    if(st->currentDigitNum == -1) {
        if(button_check_event(btn, BTN_RELEASED)) {
            if(encoder_get_delta(encoder) % 2 == 0) {
                //ok
                st->setting->value = st->currentValue;
            }
            encoder_zero(encoder);
            return false;
        }
    }
    return true;
}


void update_UI_state(UiState_t* ui, Encoder_t* encoder, Button_t* encoderBtn) {

    if(ui->setterActive) {
        if(setter_poll(&ui->setter, encoder, encoderBtn) == false) {
            ui->setterActive = false;
        }
    }
    else {
        switch(ui->mainState) {
            case UI_HOME:
                if(button_check_event(encoderBtn, BTN_RELEASED)) {
                    ui->setterActive = true;
                    setter_begin(&ui->setter, &setTemp, encoder);
                }
                if(button_check_event(encoderBtn, BTN_LONG_PRESS)) {
                    ui->mainState = UI_MENU;
                    encoder_zero(encoder);
                }
                break;
            case UI_MENU:
                if(button_check_event(encoderBtn, BTN_RELEASED)) {
                    if(encoder_get_delta(encoder) % MENU_OPTIONS_NUM == 0) { //PID
                        ui->mainState = UI_MENU_PID;
                        encoder_zero(encoder);
                    }
                    if(encoder_get_delta(encoder) % MENU_OPTIONS_NUM == 1) { //Back
                        ui->mainState = UI_HOME;
                        encoder_zero(encoder);
                    }
                }
                break;
            case UI_MENU_PID:
                if(button_check_event(encoderBtn, BTN_RELEASED)) {
                    int selected = encoder_get_delta(encoder) % 4;
                    if(selected == 3) {  //Back
                        ui->mainState = UI_MENU;
                        encoder_zero(encoder);
                    }
                    else {
                        ui->setterActive = true;
                        setter_begin(&ui->setter, &pid_settings[selected], encoder);
                    }
                    
                }
        }
    }
}

bool update_UI_str(char* firstLine, char* secondLine, const UiState_t* ui, const Encoder_t* encoder) {
    char l1[17];
    char l2[17];
    bool h1, h2, h3, fan; //to do: updatelni ezeket
    h1 = h2 = h3 = fan = true;
    /*
    h1 = (HAL_GPIO_ReadPin(H1_GPIO_Port, H1_Pin) == GPIO_PIN_SET);
    h2 = (HAL_GPIO_ReadPin(H2_GPIO_Port, H2_Pin) == GPIO_PIN_SET);
    h3 = (HAL_GPIO_ReadPin(H3_GPIO_Port, H3_Pin) == GPIO_PIN_SET);
    fan = (HAL_GPIO_ReadPin(FAN_GPIO_Port, FAN_Pin) == GPIO_PIN_SET);
    */

    double temps[4] = {69.1, 132.4, 145.5, 111.2};

    if(ui->setterActive) {
        snprintf(l1, sizeof(l1), ui->setter.setting->printfStr, ui->setter.currentValue);
            if(ui->setter.currentDigitNum == -1) {
                if(encoder_get_delta(encoder)%2 == 0)
                    snprintf(l2, sizeof(l2), ">OK      Cancel");
                else
                    snprintf(l2, sizeof(l2), " OK     >Cancel");
            }
            else {
                snprintf(l2, sizeof(l2), "                ");
                int k = ui->setter.setting->digits - 1 - ui->setter.currentDigitNum;
                if(k > ui->setter.setting->digits - ui->setter.setting->decimalPlaces - 1)
                    k++;
                k += ui->setter.setting->firstDigitOffset;
                if(k >= 0 && k <= sizeof(l2) - 1)
                    l2[k] = '^';

            }
    }
    else {
        switch(ui->mainState) {
            case UI_HOME:
                if(encoder_get_delta(encoder)%2 == 0) {
                    snprintf(l1, sizeof(l1), "Set: %.1f*C", setTemp.value); //° karaktert valahogy meg kene oldani
                    snprintf(l2, sizeof(l2), "%5.1f*C %c %c %c %c", setTemp.value, h1?'1':' ', h2?'2':' ', h3?'3':' ', fan?'F':' ');
                }
                else {
                    snprintf(l1, sizeof(l1), "%5.1f*C  %5.1f*C", temps[0], temps[1]);
                    snprintf(l2, sizeof(l2), "%5.1f*C  %5.1f*C", temps[2], temps[3]);
                }
                break;
            case UI_MENU:
                snprintf(l1, sizeof(l1), " PID settings");
                snprintf(l2, sizeof(l2), " Back");
                if(encoder_get_delta(encoder) % MENU_OPTIONS_NUM == 0)
                    l1[0] = '>';
                else
                    l2[0] = '>';
                break;
            case UI_MENU_PID:
                int selected = encoder_get_delta(encoder) % 4;
                int next = (selected + 1) % 4;
                snprintf(l1, sizeof(l1), ">%s", selected == 3 ? "Back" : pid_settings[selected].name);
                snprintf(l2, sizeof(l2), " %s", next == 3 ? "Back" : pid_settings[next].name);
        }
    }

    if(!(strcmp(l1, firstLine) || strcmp(l2, secondLine)))
        return false;
    strcpy(firstLine, l1);
    strcpy(secondLine, l2);
    return true;
}

void ui_get_settings(double* temp, double* P, double* I, double* D)
{
    if(temp != NULL)
        *temp = setTemp.value;
    if(P != NULL && I != NULL && D != NULL) {
        *P = pid_settings[0].value;
        *I = pid_settings[1].value;
        *D = pid_settings[2].value;
    }
}