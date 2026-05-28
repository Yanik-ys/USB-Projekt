/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32g4xx_hal.h"

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
#define MCU_ADC_UF200_Pin GPIO_PIN_0
#define MCU_ADC_UF200_GPIO_Port GPIOA
#define MCU_ADC_VBAT_Pin GPIO_PIN_1
#define MCU_ADC_VBAT_GPIO_Port GPIOA
#define MCU_ADC_VSUPPLY_UF200_Pin GPIO_PIN_2
#define MCU_ADC_VSUPPLY_UF200_GPIO_Port GPIOA
#define DISP_SPI_SCL_Pin GPIO_PIN_5
#define DISP_SPI_SCL_GPIO_Port GPIOA
#define DISP_SPI_MISO_Pin GPIO_PIN_6
#define DISP_SPI_MISO_GPIO_Port GPIOA
#define DISP_SPI_MOSI_Pin GPIO_PIN_7
#define DISP_SPI_MOSI_GPIO_Port GPIOA
#define DISP_SPI_CS_Pin GPIO_PIN_0
#define DISP_SPI_CS_GPIO_Port GPIOB
#define DISP_RES_Pin GPIO_PIN_1
#define DISP_RES_GPIO_Port GPIOB
#define DISP_D_C_Pin GPIO_PIN_2
#define DISP_D_C_GPIO_Port GPIOB
#define DISP_LED_Pin GPIO_PIN_11
#define DISP_LED_GPIO_Port GPIOB
#define LT1377_ON_OFF_Pin GPIO_PIN_13
#define LT1377_ON_OFF_GPIO_Port GPIOB
#define CTP_RST_Pin GPIO_PIN_15
#define CTP_RST_GPIO_Port GPIOB
#define CTP_I2C_SDA_Pin GPIO_PIN_8
#define CTP_I2C_SDA_GPIO_Port GPIOA
#define CTP_I2C_SCL_Pin GPIO_PIN_9
#define CTP_I2C_SCL_GPIO_Port GPIOA
#define DISP_CTP_INT_Pin GPIO_PIN_10
#define DISP_CTP_INT_GPIO_Port GPIOA
#define DISP_CTP_INT_EXTI_IRQn EXTI15_10_IRQn
#define SHT30A_SCL_Pin GPIO_PIN_15
#define SHT30A_SCL_GPIO_Port GPIOA
#define SHT30A_SDA_Pin GPIO_PIN_7
#define SHT30A_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
