#include "ui.h"
#include "main.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

double setTemperature = 81.5;

void update_UI_state(UiState_t* ui, uint32_t encoderPos, Button_t* encoderBtn) {

    switch(ui->mainState) {
        case UI_HOME:
            if(check_button_event(encoderBtn, BTN_RELEASED)) {
                ui->mainState = UI_SETTEMP;
                ui->substate = 0;
                ui->newSetValue = setTemperature;
                ui->encoderZero = encoderPos - (((int)ui->newSetValue)/10); //elkezdjuk allitani a felso 2 szamjegyet
            }
            break;
        case UI_SETTEMP:
            switch(ui->substate) {
                case 0:
                    ui->newSetValue = (int32_t)(encoderPos - ui->encoderZero) * 10 + fmod(ui->newSetValue, 10.0);
                    if(check_button_event(encoderBtn, BTN_RELEASED)) {
                        ui->substate = 1;
                        ui->encoderZero = encoderPos - (ui->newSetValue - ((int)ui->newSetValue/10)*10)*2; //also ket helyiertek, 0,5C felbontassal
                    }
                    break;
                case 1:
                    ui->newSetValue = ((int)ui->newSetValue/10)*10 + (int32_t)(encoderPos - ui->encoderZero)*0.5;
                    if(check_button_event(encoderBtn, BTN_RELEASED)) {
                        ui->substate = 2;
                        ui->encoderZero = encoderPos;
                    }
                    break;
                case 2:
                    if(check_button_event(encoderBtn, BTN_RELEASED)) {
                        if((encoderPos - ui->encoderZero)%2 == 0)  {//Ok, egyébként cancel
                            setTemperature = ui->newSetValue;
                        }
                        ui->mainState = UI_HOME;
                        ui->encoderZero = encoderPos;
                    }
                    break;
                default:
                    Error_Handler();
            }
        case UI_MENU:
            break;
    }
}

bool update_UI_str(char* firstLine, char* secondLine, const UiState_t* ui, uint32_t encoderPos) {
    char l1[17];
    char l2[17];
    bool h1, h2, h3, fan; //to do: updatelni ezeket
    h1 = h2 = h3 = fan = true;
    double temps[4] = {69.1, 132.4, 145.5, 111.2};

    switch(ui->mainState) {
        case UI_HOME:
            if((encoderPos - ui->encoderZero)%2 == 0) {
                snprintf(l1, sizeof(l1), "Set: %.1f*C", setTemperature); //° karaktert valahogy meg kene oldani
                snprintf(l2, sizeof(l2), "%5.1f*C %c %c %c %c", setTemperature, h1?'1':' ', h2?'2':' ', h3?'3':' ', fan?'F':' ');
            }
            else {
                snprintf(l1, sizeof(l1), "%5.1f*C  %5.1f*C", temps[0], temps[1]);
                snprintf(l2, sizeof(l2), "%5.1f*C  %5.1f*C", temps[2], temps[3]);
            }
            break;
        case UI_SETTEMP:
            snprintf(l1, sizeof(l1), "Set: %5.1f*C", ui->newSetValue);
            switch(ui->substate) {
                case 0:
                    snprintf(l2, sizeof(l2), "     ^^");
                    break;
                case 1:
                    snprintf(l2, sizeof(l2), "       ^ ^");
                    break;
                case 2:
                    if((encoderPos - ui->encoderZero)%2 == 0)
                        snprintf(l2, sizeof(l2), ">OK      Cancel");
                    else
                        snprintf(l2, sizeof(l2), " OK     >Cancel");
                    break;
                default:
                    Error_Handler();
            }
        
    }

    if(!(strcmp(l1, firstLine) || strcmp(l2, secondLine)))
        return false;
    strcpy(firstLine, l1);
    strcpy(secondLine, l2);
    return true;
}