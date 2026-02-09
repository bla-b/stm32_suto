#ifndef ADS124S08_H
#define ADS124S08_H


typedef enum {ADC_SWITCH_INPUT, ADC_WAIT_FILTER_SETTLE, ADC_WAIT_FOR_DREADY} adcState_t;

void ads124s08_getTemps(double arrayOf4temps[]);
void ads124s08_poll();
void ads124s08_init();


#endif