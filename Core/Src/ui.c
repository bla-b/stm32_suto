#include "ui.h"
#include "ads124s08.h"
#include "buttons.h"
#include "main.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
/* Gemini:
Revised Rating: 1/10
Verdict: This code works by accident. It is inefficient, buggy, and dangerous. If this is for a heater
(which the PID suggests), I wouldn't trust it to toast bread, let alone control an industrial process.
*/


// Example Flash Page (Check your linker script/memory map)
// STM32L552RCT6 has 256KB or 512KB Flash. 
// Let's use the last page of Bank 2 (e.g., Page 127 for 256KB device)
#define SETTINGS_FLASH_ADDR   0x0803F800 
#define SETTINGS_FLASH_PAGE   63
#define SETTINGS_FLASH_BANK   FLASH_BANK_2

#define MENU_OPTIONS_NUM (2)

static Setting_t setTemp = {
    .name = "",
    .value = 81.5,
    .maximum = 200.0,
    .digits = 4,
    .decimalPlaces = 1,
    .printfStr = "Set: %5.1f\xDF" "C",
    .firstDigitOffset = 5
};

static Setting_t pid_settings[3] = {
    {
        .name = "P",
        .value = 0.1,
        .maximum = 10.0,
        .digits = 4,
        .decimalPlaces = 3,
        .printfStr = "P: %5.3f",
        .firstDigitOffset = 3
    },
    {
        .name = "I",
        .value = 0.0,
        .maximum = 10.0,
        .digits = 4,
        .decimalPlaces = 3,
        .printfStr = "I: %5.3f",
        .firstDigitOffset = 3
    },
    {
        .name = "D",
        .value = 0.0,
        .maximum = 10.0,
        .digits = 4,
        .decimalPlaces = 3,
        .printfStr = "D: %5.3f",
        .firstDigitOffset = 3
    }
};

static inline int ten_to_the_pow(int exponent) {
    int num = 1;
    for(int i = 0; i < exponent; i++) {
        num *= 10;
    }
    return num;
}

static inline int pos_mod(int x, int n) {
    return (x % n + n) % n;
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
            else if(st->currentValue > st->setting->maximum) { //legvegen, ha nagyobb lenne mint a maximum
                st->currentValue = st->setting->maximum;
            }

            encoder_zero(encoder);
        }
    }
    if(st->currentDigitNum == -1) {
        if(button_check_event(btn, BTN_RELEASED)) {
            if(encoder_get_delta(encoder) % 2 == 0) {
                //ok
                if(st->setting->value != st->currentValue) {
                    st->setting->value = st->currentValue;
                    ui_Save_Settings();
                }
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
                    if(pos_mod(encoder_get_delta(encoder), MENU_OPTIONS_NUM) == 0) { //PID
                        ui->mainState = UI_MENU_PID;
                        encoder_zero(encoder);
                    }
                    if(pos_mod(encoder_get_delta(encoder), MENU_OPTIONS_NUM) == 1) { //Back
                        ui->mainState = UI_HOME;
                        encoder_zero(encoder);
                    }
                }
                break;
            case UI_MENU_PID:
                if(button_check_event(encoderBtn, BTN_RELEASED)) {
                    int selected = pos_mod(encoder_get_delta(encoder), 4);
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
    bool h1, h2, h3, fan;
    //h1 = h2 = h3 = fan = true;
    
    h1 = (HAL_GPIO_ReadPin(H1_GPIO_Port, H1_Pin) == GPIO_PIN_SET);
    h2 = (HAL_GPIO_ReadPin(H2_GPIO_Port, H2_Pin) == GPIO_PIN_SET);
    h3 = (HAL_GPIO_ReadPin(H3_GPIO_Port, H3_Pin) == GPIO_PIN_SET);
    fan = (HAL_GPIO_ReadPin(FAN_GPIO_Port, FAN_Pin) == GPIO_PIN_SET);
    

    double temps[4] = {69.1, 132.4, 145.5, 111.2};
    ads124s08_getTemps(temps);
    double averageTemp = ads124s08_getAvgTemp();

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
                    snprintf(l1, sizeof(l1), "Set: %.1f%cC", setTemp.value, 0xDF); //° karaktert
                    snprintf(l2, sizeof(l2), "%5.1f%cC %c %c %c %c", averageTemp, 0xDF, h1?'1':' ', h2?'2':' ', h3?'3':' ', fan?'F':' ');
                }
                else {
                    snprintf(l1, sizeof(l1), "%5.1f%cC  %5.1f%cC", temps[0], 0xDF, temps[1], 0xDF);
                    snprintf(l2, sizeof(l2), "%5.1f%cC  %5.1f%cC", temps[2], 0xDF , temps[3], 0xDF);
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
                int selected = pos_mod(encoder_get_delta(encoder), 4);
                int next = pos_mod((selected + 1), 4);
                snprintf(l1, sizeof(l1), ">%s", selected == 3 ? "Back" : pid_settings[selected].name);
                snprintf(l2, sizeof(l2), " %s", next == 3 ? "Back" : pid_settings[next].name);
        }
    }

    //clear helyett:
    int firstLength = strlen(l1);
    int secondLength = strlen(l2);
    for(int i = firstLength; i < sizeof(l1) - 1; i++)
        l1[i] = ' ';
    l1[sizeof(l1) - 1] = '\0';
    for(int i = secondLength; i < sizeof(l2) - 1; i++)
        l2[i] = ' ';
    l2[sizeof(l2) - 1] = '\0';

    if(strcmp(l1, firstLine) == 0 && strcmp(l2, secondLine) == 0)
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

HAL_StatusTypeDef ui_Save_Settings() {
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef eraseInitStruct;
    uint32_t PageError;

    StoredSettings_t __attribute__((aligned(8))) newSettings = {
        .setTemp = setTemp.value,
        .Kp = pid_settings[0].value,
        .Ki = pid_settings[1].value,
        .Kd = pid_settings[2].value,
        .writeCount = 0
    };

    StoredSettings_t *flashData = (StoredSettings_t *)SETTINGS_FLASH_ADDR;
    // Check if the Flash is empty (0xFF) or contains valid data
    if (flashData->writeCount == 0xFFFFFFFFFFFFFFFF) {
        newSettings.writeCount = 1;
    }
    else {
        newSettings.writeCount = flashData->writeCount + 1;
    }

    // 1. Disable interrupts, unlock flash
    __disable_irq();
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    // 2. Erase the page before writing
    eraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInitStruct.Banks     = SETTINGS_FLASH_BANK;
    eraseInitStruct.Page      = SETTINGS_FLASH_PAGE;
    eraseInitStruct.NbPages   = 1;

    status = HAL_FLASHEx_Erase(&eraseInitStruct, &PageError);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return status;
    }

    // 3. Program the struct data as Double Words (64-bit)
    uint64_t *dataPtr = (uint64_t *)(&newSettings);
    uint32_t address = SETTINGS_FLASH_ADDR;
    uint32_t iterations = sizeof(StoredSettings_t) / sizeof(uint64_t);

    for (uint32_t i = 0; i < iterations; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, dataPtr[i]);
        if (status != HAL_OK)
            break;
        address += 8; // Increment by 8 bytes for next Double Word
    }

    // 4. Enable interrupts, lock flash
    __enable_irq();
    HAL_FLASH_Lock();
    return status;
}

void ui_Load_Settings() { //adatok betoltese inicializalaskorh
    StoredSettings_t *flashData = (StoredSettings_t *)SETTINGS_FLASH_ADDR;
    
    // Check if the Flash is empty (0xFF) or contains valid data
    if (flashData->writeCount == 0xFFFFFFFFFFFFFFFF) {
        // Save hardcoded defaults if Flash is empty
        ui_Save_Settings();
    } else {
        // Copy from Flash to RAM
        setTemp.value = flashData->setTemp;
        pid_settings[0].value = flashData->Kp;
        pid_settings[1].value = flashData->Ki;
        pid_settings[2].value = flashData->Kd;
    }
}