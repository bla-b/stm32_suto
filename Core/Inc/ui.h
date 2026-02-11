#ifndef UI_H
#define UI_H

#include "main.h"
#include "buttons.h"

typedef enum {UI_HOME, UI_MENU, UI_MENU_PID} MainState_t;



typedef struct {
    const char* name;
    double value;
    /*Beallitashoz:*/
    const int digits;
    const int decimalPlaces;
    const double maximum;
    /*Kiirashoz:*/
    const char* printfStr;
    const int firstDigitOffset;
}Setting_t;

typedef struct {
    Setting_t* setting;
    int currentDigitNum;
    int currentDigitVal;
    int intValue;
    double currentValue;
}SetterState_t;

typedef struct {
    MainState_t mainState;
    bool setterActive;
    SetterState_t setter;
}UiState_t;

//void setter_begin(SetterState_t* st, Setting_t* setting, Encoder_t* encoder);

/*returns false once finished*/
//bool setter_poll(SetterState_t* st, Encoder_t* encoder, Button_t* btn);


void update_UI_state(UiState_t* ui, Encoder_t* encoder, Button_t* encoderBtn);
bool update_UI_str(char* firstLine, char* secondLine, const UiState_t* ui, const Encoder_t* encoderPos);

void ui_get_settings(double* setTemp, double* P, double* I, double* D);

#endif