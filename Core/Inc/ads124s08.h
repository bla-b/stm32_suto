#ifndef ADS124S08_H
#define ADS124S08_H

#include <stdbool.h>
#include "main.h"

typedef enum {ADC_SWITCH_INPUT, ADC_WAIT_FILTER_SETTLE, ADC_WAIT_FOR_DREADY} adcState_t;

void ads124s08_getTemps(double arrayOf4temps[]);
void ads124s08_getResistances(double arrayOf4res[]);
void ads124s08_poll();
void ads124s08_init();
bool ads124s08_readingsReadyCheck();
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
extern volatile bool dataReadyFlag;

#endif