#include "ads124s08.h"
#include "main.h"
#include "stm32l5xx_hal.h"
#include "stm32l5xx_hal_gpio.h"
#include "stm32l5xx_hal_spi.h"

#include "spi.h"
#include "usbd_cdc_if.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>


/* Eredeti:
Configure microcontroller for SPI mode 1 (CPOL = 0, CPHA = 1)
Configure microcontroller GPIO for /DRDY as a falling edge triggered interrupt input
Set CS low;
Send 06;// RESET command to make sure the device is properly reset after power-up
Set CS high;
Set CS low;// Configure the device
Send 42// WREG starting at 02h address
05// Write to 6 registers
12// Select AINP = AIN1 and AINN = AIN2
0A// PGA enabled, Gain = 4
14// Continuous conversion mode, low-latency filter, 20-SPS data rate
12// Positive reference buffer enabled, negative reference buffer disabled
 // REFP0 and REFN0 reference selected, internal reference always on
07// IDAC magnitude set to 1 mA
F0;// IDAC1 set to AIN0, IDAC2 disabled
Set CS high;
Set CS low; // For verification, read back configuration registers
Send 22// RREG starting at 02h address
05// Read from 6 registers
00 00 00 00 00 00;// Send 6 NOPs for the read
Set CS high;
Set CS low;
Send 08;// Send START command to start converting in continuous conversion mode;
Set CS high;
Loop
{
Wait for DRDY to transition low;
Set CS low;
Send 12// Send RDATA command
00 00 00;// Send 3 NOPs (24 SCLKs) to clock out data
Set CS high;
}
Set CS low;
Send 0A;//STOP command stops conversions and puts the device in standby mode;
Set CS to high;
*/

// PT100 Constants (DIN/IEC 60751)
#define PT100_R0  100.0f
#define PT100_A   3.9083e-3f
#define PT100_B   -5.775e-7f

#define FILTER_SETTLE_TIME_MS (69)
#define R_REF_VAL (1620.0)

typedef struct {
    double resistance;
    double tempDegC;
    const uint8_t* pSetupData;
    const size_t setupDataSize;
    // Calibration factors (Default: slope=1.0, offset=0.0)
    float calSlope; 
    float calOffset;
} Sensor_t;
/*
T1:
F+: ain0
S+: ain1
S-: ain2

02h: 0x12  (INPMUX)
07h: 0xF0   (IDACMUX) */
uint8_t t1_setup[6] = {0x42, 0x00,0x12, 0x47, 0x00, 0xF0};
/*T2:
F+: ain3
S+: ain4
S-: ain5

02h: 0x45  (INPMUX)
07h: 0xF3   (IDACMUX)*/
uint8_t t2_setup[6] = {0x42, 0x00,0x45, 0x47, 0x00, 0xF3};
/*T3:
F+: ain6
S+: ain7
S-: ain8

02h: 0x78  (INPMUX)
07h: 0xF6   (IDACMUX)*/
uint8_t t3_setup[6] = {0x42, 0x00,0x78, 0x47, 0x00, 0xF6};
/*T4:
F+: ain9
S+: ain10
S-: ain11

02h: 0xAB  (INPMUX)
07h: 0xF9   (IDACMUX)*/
uint8_t t4_setup[6] = {0x42, 0x00,0xAB, 0x47, 0x00, 0xF9};

uint8_t setupReadCmd[2] = {0x22, 0x05};


Sensor_t sensors[4] = {{
    .resistance = 0.0, .tempDegC = 0.0,
    .calSlope = 1.0f, .calOffset = 0.0f, // Change these after calibration
    .pSetupData = t1_setup,
    .setupDataSize = sizeof(t1_setup)
}, {
    .resistance = 0.0, .tempDegC = 0.0,
    .calSlope = 1.0f, .calOffset = 0.0f,
    .pSetupData = t2_setup,
    .setupDataSize = sizeof(t2_setup)
}, {
    .resistance = 0.0, .tempDegC = 0.0,
    .calSlope = 1.0f, .calOffset = 0.0f,
    .pSetupData = t3_setup,
    .setupDataSize = sizeof(t3_setup)
}, {
    .resistance = 0.0, .tempDegC = 0.0,
    .calSlope = 1.0f, .calOffset = 0.0f,
    .pSetupData = t4_setup,
    .setupDataSize = sizeof(t4_setup)
}};

volatile bool dataReadyFlag = false;
bool allReadingsReadyFlag = false; //ez jelzi ha korbeert a 4 szenzoron


/*static void convert(Sensor_t* sensor, uint8_t rawData[]) {
    //uint32_t intData = rawData[0] + (rawData[1] << 4) + (rawData[2] << 8);
    int32_t intData = (int32_t)((rawData[0] << 24) | (rawData[1] << 16) | (rawData[2] << 8)) >> 8;
    sensor->resistance = R_REF_VAL * (intData / (4.0 * (0x01 << 23)));
    sensor->tempDegC = (sensor->resistance - 100.0) / (100.0 * 0.00385);//todo: pontosabban
}*/

//uj, gemini vibe kodolt
static void convert(Sensor_t* sensor, uint8_t rawData[]) {
    // 1. Convert Raw Bytes to Signed Integer
    int32_t intData = (int32_t)((rawData[0] << 24) | (rawData[1] << 16) | (rawData[2] << 8));
    intData = intData >> 8; // Sign extension 

    // 2. Convert to Raw Resistance
    // Formula: R_rtd = R_ref * (Code / (Gain * 2^23))
    // Gain is 4, R_REF_VAL is 1620.0
    double raw_R = R_REF_VAL * ((double)intData / (4.0 * 8388608.0));

    // 3. Apply Calibration (Fixes wire offset and Resistor tolerance)
    sensor->resistance = (raw_R * sensor->calSlope) + sensor->calOffset;

    // 4. Convert Resistance to Temperature (Callendar-Van Dusen)
    // Using float sqrtf for speed on STM32L5 (FPU)
    double R = sensor->resistance;
    
    // Check for broken sensor (Infinite resistance or 0)
    if (R < 10.0 || R > 400.0) {
        sensor->tempDegC = -999.0; // Error code
        return;
    }

    // Quadratic Formula: T = (-A + sqrt(A^2 - 4B(1 - R/R0))) / 2B
    double discriminant = (PT100_A * PT100_A) - 4.0f * PT100_B * (1.0f - (R / PT100_R0));
    
    sensor->tempDegC = (-PT100_A + sqrt(discriminant)) / (2.0f * PT100_B);
}

void ads124s08_init() {
    const uint8_t resetCmd = 0x06;
    uint8_t setupReadBuffer[6] = {6,6,6,6,6,6};
    const uint8_t setupMsg[8] = {
        0x42,// WREG starting at 02h address
        05,// Write to 6 registers
        0x12,// Select AINP = AIN1 and AINN = AIN2
        0x0A,// PGA enabled, Gain = 4
        0x34,// Single-shot conversion mode, low-latency filter, 20-SPS data rate
        0x12,// Positive reference buffer enabled, negative reference buffer disabled
            // REFP0 and REFN0 reference selected, internal reference always on
        0x07,// IDAC magnitude set to 1 mA
        0xF0// IDAC1 set to AIN0, IDAC2 disabled
    };
    //reset
    HAL_GPIO_WritePin(ADC__CS_GPIO_Port, ADC__CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi3, &resetCmd, 1, 10u);
    HAL_GPIO_WritePin(ADC__CS_GPIO_Port, ADC__CS_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
    //setup
    HAL_GPIO_WritePin(ADC__CS_GPIO_Port, ADC__CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi3, setupMsg, sizeof(setupMsg), 10u); //todo: visszaolvasni es ellenorizni
    HAL_SPI_Transmit(&hspi3, setupReadCmd, sizeof(setupReadCmd), 10u);

    HAL_SPI_Receive(&hspi3, setupReadBuffer, 6, 10u);

    HAL_GPIO_WritePin(ADC__CS_GPIO_Port, ADC__CS_Pin, GPIO_PIN_SET);

    //char msg[50];
    //snprintf(msg, sizeof(msg), "%x\n%x\n%x\n%x\n%x\n%x\n", setupReadBuffer[0], setupReadBuffer[1], setupReadBuffer[2], setupReadBuffer[3], setupReadBuffer[4], setupReadBuffer[5]);
    //CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
}

void ads124s08_poll() {
    static adcState_t state = ADC_SWITCH_INPUT;
    static int selNum = 0; //eppen olvasott szenzor szama (0-tol)
    static uint32_t last_mux_switch_time = 0u;

    const uint8_t readCmd = 0x12;
    const uint8_t startCmd = 0x08;

    uint8_t rawData[3] = {0};
    bool ready = false;

    switch (state) {
        case ADC_SWITCH_INPUT:
            HAL_GPIO_WritePin(ADC__CS_GPIO_Port, ADC__CS_Pin, GPIO_PIN_RESET);
            HAL_SPI_Transmit(&hspi3, sensors[selNum].pSetupData, sensors[selNum].setupDataSize, 10u);
            HAL_GPIO_WritePin(ADC__CS_GPIO_Port, ADC__CS_Pin, GPIO_PIN_SET);
            last_mux_switch_time = HAL_GetTick();
            state = ADC_WAIT_FILTER_SETTLE;
        break;
        case ADC_WAIT_FILTER_SETTLE:
            if(HAL_GetTick() - last_mux_switch_time > FILTER_SETTLE_TIME_MS) {
                dataReadyFlag = false; //biztos ami biztos
                HAL_GPIO_WritePin(ADC__CS_GPIO_Port, ADC__CS_Pin, GPIO_PIN_RESET);
                HAL_SPI_Transmit(&hspi3, &startCmd, 1, 10u);
                HAL_GPIO_WritePin(ADC__CS_GPIO_Port, ADC__CS_Pin, GPIO_PIN_SET);
                state = ADC_WAIT_FOR_DREADY;
            }

        break;
        case ADC_WAIT_FOR_DREADY:
            ready = HAL_GPIO_ReadPin(ADC__DRDY_GPIO_Port, ADC__DRDY_Pin) == GPIO_PIN_RESET;
            if(dataReadyFlag) { //read data
                dataReadyFlag = false;
                HAL_GPIO_WritePin(ADC__CS_GPIO_Port, ADC__CS_Pin, GPIO_PIN_RESET);
                HAL_SPI_Transmit(&hspi3, &readCmd, 1, 10u);
                HAL_SPI_Receive(&hspi3, rawData, 3, 10u);
                HAL_GPIO_WritePin(ADC__CS_GPIO_Port, ADC__CS_Pin, GPIO_PIN_SET);
                convert(&sensors[selNum], rawData);
                state = ADC_SWITCH_INPUT;
                selNum = (selNum + 1) % 4;
                if(selNum == 0)
                    allReadingsReadyFlag = true;
        }
    }
}

void ads124s08_getTemps(double arrayOf4temps[]) {
    for(int i = 0; i < 4; i++) {
        arrayOf4temps[i] = sensors[i].tempDegC;
    }
}

bool ads124s08_readingsReadyCheck() {
    if(allReadingsReadyFlag == true) {
        allReadingsReadyFlag = false;
        return true;
    }
    return false;
}
/*
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    //Check if the interrupt came from the correct pin
    if (GPIO_Pin == ADC__DRDY_Pin) 
    {
        dataReadyFlag = true; 
    }
}
*/