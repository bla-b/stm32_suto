#ifndef TEMP_CONTROL_H
#define TEMP_CONTROL_H


typedef struct {
    int power_w;
    GPIO_PinState H1state;
    GPIO_PinState H2state;
    GPIO_PinState H3state;
} PowerLevel_t;


void tempctrl_pid_loop(double temps[4]);


#endif