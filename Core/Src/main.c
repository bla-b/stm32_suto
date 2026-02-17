/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "icache.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "buttons.h"
#include "ui.h"
#include "temp_control.h"
#include "ads124s08.h"
#include "i2c_lcd.h"
#include "usbd_cdc_if.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

I2C_LCD_HandleTypeDef lcd1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/*usb-re kiiiras kijelzo nelkul teszteleshez*/
void display_write(char* firstline, char* secondline)
{
  if(firstline == NULL || secondline == NULL || strlen(firstline) > 16 || strlen(secondline) > 16)
    Error_Handler();
  /* Check if USB is connected and configured by the host */
  if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) 
  {
    char msg[50];
    snprintf(msg, sizeof(msg), "\n\n\n\n\r%s\r%s\r", firstline, secondline);
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
  }
}
void usb_log() {
  double temps[4] = {0};
  double res[4] = {0};
  char buffer[50];
  char* end = "\n\n\n";
  if (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) 
  {
    for(int i = 0; i < 4; i++) {
      snprintf(buffer, sizeof(buffer), "%d.: %f C, resistance: %f \n", i + 1, temps[i], res[i]);
      CDC_Transmit_FS((uint8_t*)buffer, strlen(buffer));
    }
    CDC_Transmit_FS((uint8_t*)end, strlen(end));
  }
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

    Button_t start_stop_btn = {
    .port = START_BTN_GPIO_Port,
    .pin = START_BTN_Pin,
    .activeState = GPIO_PIN_RESET,
    .state = BUTTON_INACTIVE,
    .event = BTN_NO_EVENT,
    .last_time = 0u
  };
  Button_t encoder_btn = {
    .port = ROT_BTN_GPIO_Port,
    .pin = ROT_BTN_Pin,
    .activeState = GPIO_PIN_RESET,
    .state = BUTTON_INACTIVE,
    .event = BTN_NO_EVENT,
    .last_time = 0u
  };
  Encoder_t encoder = {
    .timerHandle = &htim2
  };

  uint32_t u32BlinkTimer = 0u;

  UiState_t uiState = {
    .mainState = UI_HOME
  };

  char firstLine[17] = {0};
  char secondLine[17] = {0};
  bool isActive = false;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  //gemini lcd fix, ha nem kell ki lehet venni:
  HAL_Delay(100);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USB_Device_Init();
  MX_ICACHE_Init();
  MX_I2C1_Init();
  MX_SPI3_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  ads124s08_init();
  // --- LCD INDÍTÁS ---
  lcd1.hi2c = &hi2c1;       // Az I2C busz hozzárendelése (fontos: hi2c1)
  lcd1.address = 0x4E;      // Cím beállítása (Ha nem megy, próbáld: 0x27)
  lcd_init(&lcd1);          // Kijelző bekapcsolása
  // Opcionális: Indulási üzenet teszteléshez
  lcd_gotoxy(&lcd1, 0, 0);
  lcd_puts(&lcd1, "System Ready");
  HAL_Delay(1000);          // Várunk 1 mp-et, hogy lásd
  lcd_clear(&lcd1);         // Letöröljük

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) //todo: watchdog prescaler!!!!!
  {
    button_poll(&start_stop_btn);
    button_poll(&encoder_btn);

    if(button_check_event(&start_stop_btn, BTN_RELEASED)) {
      HAL_GPIO_TogglePin(ACTIVE_LED_GPIO_Port, ACTIVE_LED_Pin);
      isActive = !isActive;
    }

    ads124s08_poll();
    if(ads124s08_readingsReadyCheck()) {
      tempctrl_pid_loop(isActive); //csak akkor kell meghivni ha korbeert a 4 szenzoron
      usb_log();
    }

    //update ui
    update_UI_state(&uiState, &encoder, &encoder_btn);
    //update display (csak ha valami valtozott)
    if(update_UI_str(firstLine, secondLine, &uiState, &encoder) == true) {
      
      //display_write(firstLine, secondLine);

      // --- LCD FRISSÍTÉS ---
      //lcd_clear(&lcd1);             // Törlés (fontos a szemetelődés ellen)
      //clear helyett:
      int firstLength = strlen(firstLine);
      int secondLength = strlen(secondLine);
      for(int i = firstLength; i < sizeof(firstLine) - 1; i++)
        firstLine[i] = ' ';
      firstLine[sizeof(firstLine) - 1] = '\n';
      for(int i = secondLength; i < sizeof(secondLine) - 1; i++)
        secondLine[i] = ' ';
      secondLine[sizeof(secondLine) - 1] = '\n';

      lcd_gotoxy(&lcd1, 0, 0);      // 1. sor eleje
      lcd_puts(&lcd1, firstLine);   // 1. szöveg
      
      lcd_gotoxy(&lcd1, 0, 1);      // 2. sor eleje
      lcd_puts(&lcd1, secondLine);  // 2. szöveg

    }
    
    // Get the direction (0 = Up, 1 = Down)
    //bool direction = __HAL_TIM_IS_TIM_COUNTING_DOWN(&htim1);

    //Blinker
    if(HAL_GetTick() - u32BlinkTimer > 500u) {
      //HAL_GPIO_TogglePin(STA_LED_GPIO_Port, STA_LED_Pin);
      u32BlinkTimer = HAL_GetTick();
    }

    // Frissítjük a Watchdog-ot                       // CubeMx ben be kell állítani a watchdogot
    HAL_IWDG_Refresh(&hiwdg); 

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    /* Check if the interrupt came from the correct pin */
    if (GPIO_Pin == ADC__DRDY_Pin) 
    {
        dataReadyFlag = true; 
        HAL_GPIO_TogglePin(STA_LED_GPIO_Port, STA_LED_Pin);
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
