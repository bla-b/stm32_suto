#include "temp_control.h"
#include "main.h"
#include "ads124s08.h"
#include "stm32l5xx_hal_gpio.h"
#include "ui.h"

//H1 = 2.4kW, H2 = 2kW, H3 = 1.2kW //H1 most nincs
#define NO_H1

#define OUT_PWR_HYSTERESYS (100) //[W]
#define INTEGRAL_LIMIT (200.0) //[degC * s]
#define FAN_ON_T_DIFF_THRESHOLD (4.0) //[degC]
#define FAN_OFF_T_DIFF_THRESHOLD (3.5) //[degC]


#define ABS(x) ((x) > 0 ? (x) : -(x))

#ifndef NO_H1
#define POWER_LEVELS_NUM (8)
const PowerLevel_t pwrLevels[POWER_LEVELS_NUM] = {
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
#endif
#ifdef NO_H1
#define POWER_LEVELS_NUM (4)

const PowerLevel_t pwrLevels[POWER_LEVELS_NUM] = {
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
        .power_w = 3200,
        .H1state = GPIO_PIN_RESET,
        .H2state = GPIO_PIN_SET,
        .H3state = GPIO_PIN_SET
    }
};

#endif

void tempctrl_pid_loop(bool isActive) {
    static uint32_t lastTime = 0;
    static double lastError = 0.0;
    static double integral = 0.0;
    static int lastPwrOut = 0; //last calculated power

    double error, dt, derivative;
    int pwrOut;
    uint32_t time = HAL_GetTick();

    if(isActive == false) { // ha nincs bekapcsolva:
        HAL_GPIO_WritePin(H1_GPIO_Port, H1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(H2_GPIO_Port, H2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(H3_GPIO_Port, H3_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_RESET);
        lastPwrOut = 0;
        return;
    }

    //copy settings:
    double P, I, D;
    double setTemp;
    ui_get_settings(&setTemp, &P, &I, &D);

    //get temperature:
    double temps[4];
    ads124s08_getTemps(temps);
    double avgTemp = ads124s08_getAvgTemp();

    // --- VÉSZLEÁLLÍTÓ (TÚLMELEGEDÉS VÉDELEM) ---
    // Végignézzük mind a 4 szenzort.
    for(int i = 0; i < 4; i++) {
        if(temps[i] > 250.0 && temps[i] < 990.0) { // 250 fok felett vészleállítás! Ezt átírhatod a sütőd max limitjére.
            
            // Minden fűtést és ventilátort azonnal lekapcsolunk!
            HAL_GPIO_WritePin(H1_GPIO_Port, H1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(H2_GPIO_Port, H2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(H3_GPIO_Port, H3_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_RESET);
            
            lastPwrOut = 0; // Nullázzuk a memóriát is
            
            return; // KILÉPÜNK! A PID számítás már le sem fut, a relék kikapcsolva maradnak.
        }
    }
    // -------------------------------------------

    //calculate error:
    error = setTemp - avgTemp;
    //calculate delta t:
    dt = (time - lastTime) / 1000.0; //elapsed time in s
    //if(dt > 2.0)
    //     dt = 2.0;

    //derivative:
    if(dt > 0.01)
        derivative = (error - lastError) / dt;
    else
        derivative = 0.0;

    //integral
    if (ABS(error) < 5.0) {
        integral += error * dt;
    } else {
        integral = 0; 
    }
    // integral += error * dt;
    integral = (integral > INTEGRAL_LIMIT) ? INTEGRAL_LIMIT : integral; //limit integral
    integral = (integral < 0.0) ? 0.0 : integral; //ne menjen negativba mert az minek

    //calculate output:
    pwrOut = (int)((P * error + I / 10.0 * integral + D * derivative * 10.0) * 1000.0); //ezt lehet meg kell majd szorozni mondjuk 1000-rel

    //find best power level
    if(ABS(pwrOut - lastPwrOut) > OUT_PWR_HYSTERESYS) { //ez most igy szar de nem kell//hiszterizis, hogy ne kapcsolgassanak feleslegesen a relek, majd kiderul kell-e
        int newLvlIndex = 0;
        for(int i = 1; i < POWER_LEVELS_NUM; i++) {
            if(pwrOut > ((pwrLevels[i].power_w + pwrLevels[i - 1].power_w) / 2)) //megnezzuk melyikhez van a legkozelebb
                newLvlIndex++;
        }

        lastPwrOut = pwrOut;//fontos hogy csak akkor allitsuk at, ha megvolt a frissites

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
        if(temps[i] > 998.0)
            continue; //skip disconnected sensor
        if(temps[i] > temps[maxIndex])
            maxIndex = i;
        if(temps[i] < temps[minIndex])
            minIndex = i;
    }
    maxTdiff = temps[maxIndex] - temps[minIndex];
    if(maxTdiff > FAN_ON_T_DIFF_THRESHOLD && !fanIsOn) {
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_SET);
    }
    else if(maxTdiff < FAN_OFF_T_DIFF_THRESHOLD && fanIsOn) {
        HAL_GPIO_WritePin(FAN_GPIO_Port, FAN_Pin, GPIO_PIN_RESET);
    }


    //
    lastTime = time;
    lastError = error;
}



