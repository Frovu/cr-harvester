/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdarg.h>
#include "counter.h"

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
void debug_printf(const char * fmt, ...);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BOARD_LED_Pin GPIO_PIN_13
#define BOARD_LED_GPIO_Port GPIOC
#define W5500_RESET_Pin GPIO_PIN_1
#define W5500_RESET_GPIO_Port GPIOA
#define W5500_CS_Pin GPIO_PIN_4
#define W5500_CS_GPIO_Port GPIOA
#define AT25_CS_Pin GPIO_PIN_0
#define AT25_CS_GPIO_Port GPIOB
#define LED_DATA_Pin GPIO_PIN_12
#define LED_DATA_GPIO_Port GPIOA
#define LED_ERROR_Pin GPIO_PIN_15
#define LED_ERROR_GPIO_Port GPIOA
#define BUTTON_RESET_Pin GPIO_PIN_4
#define BUTTON_RESET_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
#define LED_ON(led) \
  HAL_GPIO_WritePin(led##_GPIO_Port, led##_Pin, GPIO_PIN_SET)
#define LED_OFF(led) \
  HAL_GPIO_WritePin(led##_GPIO_Port, led##_Pin, GPIO_PIN_RESET)
#define LED_BLINK(led, delay)     \
	LED_ON(led);                    \
	HAL_Delay(delay);               \
	LED_OFF(led)
#define LED_BLINK_INV(led, delay) \
	LED_OFF(led);                   \
	HAL_Delay(delay);               \
	LED_ON(led)


#define DEBUG_UART

#define DEBUG_FMT_MAX_SIZE 256

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
