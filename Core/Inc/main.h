/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ROT_A_Pin GPIO_PIN_0
#define ROT_A_GPIO_Port GPIOA
#define ROT_B_Pin GPIO_PIN_1
#define ROT_B_GPIO_Port GPIOA
#define ROT_BTN_Pin GPIO_PIN_2
#define ROT_BTN_GPIO_Port GPIOA
#define ACTIVE_LED_Pin GPIO_PIN_3
#define ACTIVE_LED_GPIO_Port GPIOA
#define START_BTN_Pin GPIO_PIN_4
#define START_BTN_GPIO_Port GPIOA
#define H1_Pin GPIO_PIN_12
#define H1_GPIO_Port GPIOB
#define FAN_Pin GPIO_PIN_13
#define FAN_GPIO_Port GPIOB
#define H2_Pin GPIO_PIN_14
#define H2_GPIO_Port GPIOB
#define H3_Pin GPIO_PIN_15
#define H3_GPIO_Port GPIOB
#define STA_LED_Pin GPIO_PIN_8
#define STA_LED_GPIO_Port GPIOA
#define ADC__DRDY_Pin GPIO_PIN_15
#define ADC__DRDY_GPIO_Port GPIOA
#define ADC_SCK_Pin GPIO_PIN_10
#define ADC_SCK_GPIO_Port GPIOC
#define ADC_MISO_Pin GPIO_PIN_11
#define ADC_MISO_GPIO_Port GPIOC
#define ADC_MOSI_Pin GPIO_PIN_12
#define ADC_MOSI_GPIO_Port GPIOC
#define ADC__CS_Pin GPIO_PIN_2
#define ADC__CS_GPIO_Port GPIOD
#define DISP_SCL_Pin GPIO_PIN_6
#define DISP_SCL_GPIO_Port GPIOB
#define DISP_SDA_Pin GPIO_PIN_7
#define DISP_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
