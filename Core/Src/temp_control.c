#include "temp_control.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "ui.h"

//H1 = 2.4kW, H2 = 2kW, H3 = 1.2kW
#define OUT_PWR_HYSTERESYS (0) //[W]
#define FAN_ON_T_DIFF_THRESHOLD (4.0) //[degC]
#define FAN_OFF_T_DIFF_THRESHOLD (3.5) //[degC]
#define ABS(x) (x > 0 ? x : -x)

const PowerLevel_t pwrLevels[8] = {
    {
        .power_w = 0,
        .H1state = GPIO_PIN_RESET,
        .H2state = GPIO_PIN_RESET,
        .H3state = GPIO_PIN_RESET
    },
    {
        .power_w = 1200,
        .H1state = GPIO_PIN_RESET,
        .H2state = GPIO_PIN_RESET,
        .H3state = GPIO_PIN_SET
    },
    {
        .power_w = 2000,
        .H1state = GPIO_PIN_RESET,
        .H2state = GPIO_PIN_SET,
        .H3state = GPIO_PIN_RESET
    },
    {
        .power_w = 2400,
        .H1state = GPIO_PIN_SET,
        .H2state = GPIO_PIN_RESET,
        .H3state = GPIO_PIN_RESET
    },
    {
        .power_w = 3200,
        .H1state = GPIO_PIN_RESET,
        .H2state = GPIO_PIN_SET,
        .H3state = GPIO_PIN_SET
    },
    {
        .power_w = 3600,
        .H1state = GPIO_PIN_SET,
        .H2state = GPIO_PIN_RESET,
        .H3state = GPIO_PIN_SET
    },
    {
        .power_w = 4400,
        .H1state = GPIO_PIN_SET,
        .H2state = GPIO_PIN_SET,
        .H3state = GPIO_PIN_RESET
    },
    {
        .power_w = 5600,
        .H1state = GPIO_PIN_SET,
        .H2state = GPIO_PIN_SET,
        .H3state = GPIO_PIN_SET
    }
};

void tempctrl_pid_loop(double temps[4]) {
    static uint32_t lastTime = 0;
    static double lastError = 0.0;
    static double integral = 0.0;
    static int lastPwrOut = 0; //last calculated power

    double error, dt, derivative;
    int pwrOut;
    uint32_t time = HAL_GetTick();

    //copy settings:
    double P, I, D;
    double setTemp;
    ui_get_settings(&setTemp, &P, &I, &D);

    //calculate error:
    double avg = temps[0] + temps[1] + temps[2] + temps[3];
    avg /= 4.0;
    error = avg - setTemp;
    //calculate delta t:
    dt = (time - lastTime) / 1000.0; //elapsed time in s
    if(dt > 2.0)
        dt = 2.0;

    //derivative:
    if(dt > 0.01)
        derivative = (error - lastError) / dt;
    else
        derivative = 0.0;

    //integral
    integral += error * dt;
    integral = (integral > 1000.0) ? 1000.0 : integral; //limit integral
    integral = (integral < -1000.0) ? -1000.0 : integral;

    //calculate output:
    pwrOut = (int)(P * error + I * integral + D * derivative); //ezt lehet meg kell majd szorozni mondjuk 1000-rel

    //find best power level
    if(ABS(pwrOut - lastPwrOut) > OUT_PWR_HYSTERESYS) { //hiszterizis, hogy ne kapcsolgassanak feleslegesen a relek, majd kiderul kell-e
        int newLvlIndex = 0;
        for(int i = 1; i < 8; i++) {
            if(pwrOut > ((pwrLevels[i].power_w + pwrLevels[i - 1].power_w) / 2)) //megnezzuk melyikhez van a legkozelebb
                newLvlIndex++;
        }

        //set the output
        HAL_GPIO_WritePin(H1_GPIO_Port, H1_Pin, pwrLevels[newLvlIndex].H1state);
        HAL_GPIO_WritePin(H2_GPIO_Port, H2_Pin, pwrLevels[newLvlIndex].H2state);
        HAL_GPIO_WritePin(H3_GPIO_Port, H3_Pin, pwrLevels[newLvlIndex].H3state);
    }

    //Fan:
    double maxTdiff = 0;
    bool fanIsOn = (HAL_GPIO_ReadPin(FAN_GPIO_Port, FAN_Pin) == GPIO_PIN_SET);
    int minIndex = 0;
    int maxIndex = 0;
    for(int i = 1; i < 4; i++) { //find min and max temperature
        if(temps[i] > temps[maxIndex])
            maxIndex = i;
        if(temps[i] < temps[minIndex])
            minIndex = i;
    }
    maxTdiff = temps[maxIndex] - temps[minIndex];
    if(maxTdiff > FAN_ON_T_DIFF_THRESHOLD && !fanIsOn) {
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_SET);
    }
    else if(maxTdiff > FAN_OFF_T_DIFF_THRESHOLD && fanIsOn) {
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_RESET);
    }


    //
    lastTime = time;
    lastError = error;
    lastPwrOut = pwrOut;
}



