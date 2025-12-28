#ifndef UI_H
#define UI_H

#include "main.h"
#include "buttons.h"

typedef enum {UI_HOME, UI_MENU, UI_SETTEMP} MainState_t;

typedef struct {
    MainState_t mainState;
    int substate;
    int32_t encoderZero;
    double newSetValue;
}UiState_t;

void update_UI_state(UiState_t* ui, uint32_t encoderPos, Button_t* encoderBtn);
bool update_UI_str(char* firstLine, char* secondLine, const UiState_t* ui, uint32_t encoderPos);

#endif