/* USER CODE BEGIN Header */
/*------------------------------------------------------------------------------
Project information's:
        Project:    Temp_Measuring_SW_IPA_V1
        Author:     SPY
        Date:       21.10.2024
        IDE:        STM32CubeIDE 1.14.0
        Version:    1.0
------------------------------------------------------------------------------*/
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>               /* for pow() */
#include "sht3x.h"              /* for external temperature sensor */
#include "ili9341.h"            /* for display */
#include "FT6236.h"             /* for touch panel */
#include "stdio.h"              /* for sprintf() */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum menu_temp_measuring {
    START_MENU,
    HOME_MENU,
    TEMP_MENU,
    TEMP_UF200,
    TEMP_SHT30,
    VOLTAGE_MENU,
    SETTINGS_MENU
}Menu_TempMeasuring_TypeDef;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ----------------------- ADC -------------------------------- */
#define ADC_NR_OF_CHANNELS              3
#define ADC_NR_OF_CAPTURED_VALUES       10
#define ADC_BUFFER_SIZE                 (ADC_NR_OF_CHANNELS * ADC_NR_OF_CAPTURED_VALUES)
#define ADC_VREF                        2.5
#define ADC_MAX_NR                      4096

/* ----------------------- SUPPLY_UF200 ----------------------- */
#define SUPPLY_UF200_ON                 HAL_GPIO_WritePin(LT1377_ON_OFF_GPIO_Port, LT1377_ON_OFF_Pin, SET); HAL_Delay(1);
#define SUPPLY_UF200_OFF                HAL_GPIO_WritePin(LT1377_ON_OFF_GPIO_Port, LT1377_ON_OFF_Pin, RESET); HAL_Delay(1);
#define STATE_LT1377                    HAL_GPIO_ReadPin(LT1377_ON_OFF_GPIO_Port, LT1377_ON_OFF_Pin)

/* ----------------------- STANDBY_MODE ----------------------- */
#define ENTER_STANDBY_MODE              HAL_PWR_EnterSTANDBYMode();

/* ----------------------- DISPLAY --------------------------- */
#define DISPLAY_BACKLIGHT_ON            HAL_GPIO_WritePin(DISP_LED_GPIO_Port, DISP_LED_Pin, SET); HAL_Delay(1);
#define DISPLAY_BACKLIGHT_OFF           HAL_GPIO_WritePin(DISP_LED_GPIO_Port, DISP_LED_Pin, RESET); HAL_Delay(1);


/* ----------------------- VOLTAGE TO DISTANCE ---------------- */
#define DISTANCE_FUNC(voltage)          (1.52204 * voltage + 118.90031)

/* ----------------------- TOUCH AREAs ------------------------ */
/* --------- HOME --------- */
#define BUTTON_TEMP_TOUCHED             (20 < ctp_touch.x1 && ctp_touch.x1 < 220 && 70 < ctp_touch.y1 && ctp_touch.y1 < 120)
#define BUTTON_VOLTAGE_TOUCHED          (20 < ctp_touch.x1 && ctp_touch.x1 < 220 && 135 < ctp_touch.y1 && ctp_touch.y1 < 185)
#define BUTTON_SETTINGS_TOUCHED         (20 < ctp_touch.x1 && ctp_touch.x1 < 220 && 200 < ctp_touch.y1 && ctp_touch.y1 < 250)
/* --------- TEMP --------- */
#define BUTTON_UF200_TOUCHED            (60 < ctp_touch.x1 && ctp_touch.x1 < 180 && 80 < ctp_touch.y1 && ctp_touch.y1 < 140)
#define BUTTON_SHT30_TOUCHED            (60 < ctp_touch.x1 && ctp_touch.x1 < 180 && 170 < ctp_touch.y1 && ctp_touch.y1 < 230)
/* ------- SETTINGS ------- */
#define BUTTON_LT1377_TOUCHED           (60 < ctp_touch.x1 && ctp_touch.x1 < 180 && 80 < ctp_touch.y1 && ctp_touch.y1 < 140)
#define BUTTON_STANDBY_TOUCHED          (60 < ctp_touch.x1 && ctp_touch.x1 < 180 && 170 < ctp_touch.y1 && ctp_touch.y1 < 230)
/* --------- EXIT --------- */
#define BUTTON_EXIT_TOUCHED             (80 < ctp_touch.x1 && ctp_touch.x1 < 160 && 270 < ctp_touch.y1 && ctp_touch.y1 < 310)



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
sht3x_handle_t handle = {
    .i2c_handle = &hi2c1,
    .device_address = SHT3X_I2C_DEVICE_ADDRESS_ADDR_PIN_LOW
};

/* ---------- ADC1 ---------- */
volatile uint16_t adc1_measurement[ADC_BUFFER_SIZE];

/* -------- Voltages -------- */
volatile float volt_uf200;
volatile float volt_vbat;
volatile float volt_supply_uf200;

/* ------- Distance --------- */
volatile float distance_uf200;

/* ------ Temperatures ------ */
volatile float temp_uf200;
float temp_sht30a;
float humidity_sht30a;

/* ---------- ENUM ---------- */
enum ADC1_ARRAY {
    ADC_UF200 = 0,
    ADC_VBAT = 1,
    ADC_SUPPLY_UF200 = 2,
    ADC_TEMP = 3
};

enum STRING_ARRAY {
    TEMP_UF200_STRING = 0,
    TEMP_SHT30_STRING = 1,
    HUMITY_SHT30_STRING = 2
};

/* ------ String --------- */
char voltage_Data_String[3][16] = {};
char temp_Data_String[3][16] = {};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */
/* ------ format functions ------ */
volatile void format_voltage_value(float value, const char* unit, char* formatted_string, size_t size);
volatile void format_temp_value(float value, const char* unit, char* formatted_string, size_t size);
/* ------ display functions ------ */
void Dispaly_EXIT_BUTTON();
void Dispaly_HOME_MENU();
void Dispaly_TEMP_MENU();
void Dispaly_UF200_MENU();
void Dispaly_SHT30_MENU();
void Dispaly_VOLTAGE_MENU();
void Dispaly_SETTINGS_MENU();
/* ------ update functions ------ */
void Update_Dispaly_UF200_MENU();
void Update_Dispaly_SHT30_MENU();
void Update_Dispaly_VOLTAGE_MENU();
/* ------ toggle functions ------ */
void TOGGLE_LT1377();
/* ------ calculate functions ------ */
volatile float calculate_polynomial(float x);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile float calculate_temp(float x) {
    float x2 = x * x;         // x^2
    float x3 = x2 * x;        // x^3
    float x4 = x3 * x;        // x^4

    return 0.0000393460875 * x4
           - 0.023479315484 * x3
           + 5.2439117826509 * x2
           - 523.030559479747 * x
           + 19723.208350605753;
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        /* -------------------- Channel 1 MCU_ADC_UF200 -------------------- */
        /* calculate the average of the captured values */
        for (int i = 1; i < ADC_NR_OF_CAPTURED_VALUES; ++i) {
            adc1_measurement[ADC_UF200] += adc1_measurement[i * 3];
        }
        adc1_measurement[ADC_UF200] /= ADC_NR_OF_CAPTURED_VALUES;

        /* convert digital value to voltage value */
        volt_uf200 = ADC_VREF * 4.0669456 * adc1_measurement[ADC_UF200] / ADC_MAX_NR * 1.089230769;
        volt_uf200 = 0.0003 * (volt_uf200*volt_uf200) + 0.9009 * (volt_uf200) + 0.145;
        /* ----------------------------------------------------------------- */

        /* ---------------------- Distance calculation --------------------- */
        distance_uf200 = DISTANCE_FUNC(volt_uf200);
        /* ----------------------------------------------------------------- */

        /* -------------------- Temperature calculation -------------------- */
        temp_uf200 = calculate_temp(distance_uf200);
        /* ----------------------------------------------------------------- */


        /* -------------------- Channel 2 MCU_ADC_VBAT --------------------- */
        /* calculate the average of the captured values */
        for (int i = 1; i < ADC_NR_OF_CAPTURED_VALUES; ++i) {
            adc1_measurement[ADC_VBAT] += adc1_measurement[i * 3 + 1];
        }
        adc1_measurement[ADC_VBAT] /= ADC_NR_OF_CAPTURED_VALUES;

        /* convert digital value to voltage value */
        volt_vbat = ADC_VREF * 1.4202773 * adc1_measurement[ADC_VBAT] / ADC_MAX_NR * 1.0039816;
        /* ----------------------------------------------------------------- */


        /* -------------------- Channel 3  MCU_SUPPLY_UF200 ---------------- */
        /* calculate the average of the captured values */
        for (int i = 1; i < ADC_NR_OF_CAPTURED_VALUES; ++i) {
            adc1_measurement[ADC_SUPPLY_UF200] += adc1_measurement[i * 3 + 2];
        }
        adc1_measurement[ADC_SUPPLY_UF200] /= ADC_NR_OF_CAPTURED_VALUES;

        /* convert digital value to voltage value */
        volt_supply_uf200 = ADC_VREF * 6.0570739 * adc1_measurement[ADC_SUPPLY_UF200] / ADC_MAX_NR * 1.0035293;
        /* ----------------------------------------------------------------- */

    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_USB_Device_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */

  /* ------------- create & declare menu_of_device ------------ */
  Menu_TempMeasuring_TypeDef menu_of_device = HOME_MENU;

  /* ------------------- initialize display ------------------- */
  DISPLAY_BACKLIGHT_OFF
  ILI9341_Init();
  ILI9341_InvertColors(1);
  DISPLAY_BACKLIGHT_ON

  /* ----------------- initialize touch panel ----------------- */
  ft6236_init();

  /* -------------------- turn on LT1377 -------------------- */
  TOGGLE_LT1377();

  /* ------------------- draw home menu ------------------- */
  ILI9341_FillRectangle(0, 0, 240, 320, ILI9341_BLACK);
  ILI9341_FillRectangle(0, 58, 240, 3, ILI9341_WHITE);
  Dispaly_HOME_MENU();

  /* ----------------- start DMA conversion ----------------- */
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc1_measurement, ADC_BUFFER_SIZE);


  /* -------------- initialize temperature sensor ------------- */
  if (!sht3x_init(&handle)) {
  }

  sht3x_set_header_enable(&handle, false);



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
     /* read touch panel */
      if(!ctp_touch.ok) {

                ctp_touch.x1 = 0;
                ctp_touch.y1 = 0;
                ft6236_read();
      }
    /* read temperature and humidity */
      sht3x_read_temperature_and_humidity(&handle, &temp_sht30a, &humidity_sht30a);

    /* menu navigation */
      switch (menu_of_device) {
        case START_MENU:

            if(ctp_touch.x1 > 0 && ctp_touch.y1 > 0 ) {
                Dispaly_HOME_MENU();
                menu_of_device = HOME_MENU;
            }

            break;

        case HOME_MENU:

            /* SETTINGS-MENU */
            if(BUTTON_SETTINGS_TOUCHED) {
                Dispaly_SETTINGS_MENU();
                menu_of_device = SETTINGS_MENU;
            }

            /* TEMP-MENU */
            if(BUTTON_TEMP_TOUCHED) {
                Dispaly_TEMP_MENU();
                menu_of_device = TEMP_MENU;
            }

            /* VOLTAGE-MENU */
            if(BUTTON_VOLTAGE_TOUCHED) {
                Dispaly_VOLTAGE_MENU();
                menu_of_device = VOLTAGE_MENU;
            }

            break;


        case SETTINGS_MENU:
            /* LT1377 ON/OFF */
            if(BUTTON_LT1377_TOUCHED) {
                TOGGLE_LT1377();
            }

            /* Exit / back to HOME */
            if(BUTTON_EXIT_TOUCHED) {
                Dispaly_HOME_MENU();
                menu_of_device = HOME_MENU;
            }

            /* if(BUTTON_STANDBY_TOUCHED) {
                __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
                __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
                HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_A, GPIO_PIN_10);
                HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_A, GPIO_PIN_9);
                HAL_PWREx_EnableGPIOPullUp(PWR_GPIO_A, GPIO_PIN_8);
                HAL_PWREx_EnablePullUpPullDownConfig();
                HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2_LOW);
                HAL_PWR_EnterSTANDBYMode();
            } */
            break;


        case TEMP_MENU:
            /* switch to TEMP_UF200 */
            if(BUTTON_UF200_TOUCHED) {
                Dispaly_UF200_MENU();
                menu_of_device = TEMP_UF200;
            }

            /* switch to TEMP_SHT30 */
            if(BUTTON_SHT30_TOUCHED) {
                Dispaly_SHT30_MENU();
                menu_of_device = TEMP_SHT30;
            }

            /* Exit / back to HOME */
            if(BUTTON_EXIT_TOUCHED) {
                Dispaly_HOME_MENU();
                menu_of_device = HOME_MENU;
            }

            break;

        case TEMP_UF200:
            /* update display */
            Update_Dispaly_UF200_MENU();

            /* Exit / back to HOME */
            if(BUTTON_EXIT_TOUCHED) {
                Dispaly_HOME_MENU();
                menu_of_device = HOME_MENU;
            }

            break;

        case TEMP_SHT30:
            /* update display */
            Update_Dispaly_SHT30_MENU();

            /* Exit / back to HOME */
            if(BUTTON_EXIT_TOUCHED) {
                Dispaly_HOME_MENU();
                menu_of_device = HOME_MENU;
            }

            break;

        case VOLTAGE_MENU:
            /* update display */
            Update_Dispaly_VOLTAGE_MENU();

            /* Exit / back to HOME */
            if(BUTTON_EXIT_TOUCHED) {
                Dispaly_HOME_MENU();
                menu_of_device = HOME_MENU;
            }

            break;
        default:
            break;
    }

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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
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
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* EXTI15_10_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.GainCompensation = 0;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 3;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hadc1.Init.OversamplingMode = ENABLE;
  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_16;
  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_4;
  hadc1.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
  hadc1.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_247CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00B07CB4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_ENABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00B07CB4;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMAMUX1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DISP_SPI_CS_Pin|DISP_RES_Pin|DISP_D_C_Pin|DISP_LED_Pin
                          |LT1377_ON_OFF_Pin|CTP_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : DISP_SPI_CS_Pin DISP_RES_Pin DISP_D_C_Pin DISP_LED_Pin
                           LT1377_ON_OFF_Pin CTP_RST_Pin */
  GPIO_InitStruct.Pin = DISP_SPI_CS_Pin|DISP_RES_Pin|DISP_D_C_Pin|DISP_LED_Pin
                          |LT1377_ON_OFF_Pin|CTP_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : DISP_CTP_INT_Pin */
  GPIO_InitStruct.Pin = DISP_CTP_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DISP_CTP_INT_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/*------------------------------------------------------------------------------
Function name:  format_voltage_value
Input:          float value, const char* unit, char* formatted_string, size_t size
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function formats the voltage value to a string.
The value is rounded to 3 decimal places.
The unit is added to the end of the string.
-------------------------------------------------------------------------------*/
volatile void format_voltage_value(float value, const char* unit, char* formatted_string, size_t size) {
    if (value  < 10) {
        snprintf(formatted_string, size, " %2.3f %s", value, unit); // @suppress("Float formatting support")
    } else {
        snprintf(formatted_string, size, "%2.3f %s", value, unit); // @suppress("Float formatting support")
    }
}

/*------------------------------------------------------------------------------
Function name:  format_temp_value
Input:          float value, const char* unit, char* formatted_string, size_t size
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function formats the temperature value to a string.
The value is rounded to 2 decimal places.
The unit is added to the end of the string.
-------------------------------------------------------------------------------*/
volatile void format_temp_value(float value, const char* unit, char* formatted_string, size_t size) {
    if (value  < 10 && value  >= 0) {
        snprintf(formatted_string, size, "  %2.2f %s", value, unit); // @suppress("Float formatting support")
    } else if (value > 0) {
        snprintf(formatted_string, size, " %2.2f %s", value, unit); // @suppress("Float formatting support")
    } else {
        snprintf(formatted_string, size, "%2.2f %s", value, unit); // @suppress("Float formatting support")
    }
}

/*------------------------------------------------------------------------------
Function name:  Dispaly_EXIT_BUTTON
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function displays the EXIT button on the display.
-------------------------------------------------------------------------------*/
void Dispaly_EXIT_BUTTON() {
    ILI9341_FillRectangle(80, 270, 80, 40,ILI9341_COLOR565(77,77,77));
    ILI9341_WriteString(98, 281, "EXIT", Font_11x18, ILI9341_WHITE, ILI9341_COLOR565(77,77,77));

}

/*------------------------------------------------------------------------------
Function name:  Dispaly_HOME_MENU
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function displays the HOME menu on the display.
-------------------------------------------------------------------------------*/
void Dispaly_HOME_MENU() {
    ILI9341_FillRectangle(20, 10, 220, 40, ILI9341_BLACK);
    ILI9341_WriteString(98, 20, "HOME", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_FillRectangle(20, 70, 200, 50, ILI9341_COLOR565(77,77,77));
    ILI9341_FillRectangle(20, 120, 200, 15, ILI9341_BLACK);
    ILI9341_FillRectangle(20, 135, 200, 50, ILI9341_COLOR565(77,77,77));
    ILI9341_FillRectangle(20, 185, 200, 15, ILI9341_BLACK);
    ILI9341_FillRectangle(20, 200, 200, 50, ILI9341_COLOR565(77,77,77));
    ILI9341_FillRectangle(20, 250, 200, 65, ILI9341_BLACK);
    ILI9341_WriteString(60, 86, "Temperature", Font_11x18, ILI9341_WHITE, ILI9341_COLOR565(77,77,77));
    ILI9341_WriteString(80, 151, "Voltage", Font_11x18, ILI9341_WHITE, ILI9341_COLOR565(77,77,77));
    ILI9341_WriteString(80, 216, "Settings", Font_11x18, ILI9341_WHITE, ILI9341_COLOR565(77,77,77));
}

/*------------------------------------------------------------------------------
Function name:  Dispaly_TEMP_MENU
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function displays the TEMP menu on the display, where the user can choose between UF200 and SHT30.
-------------------------------------------------------------------------------*/
void Dispaly_TEMP_MENU() {
    ILI9341_FillRectangle(20, 10, 220, 40, ILI9341_BLACK);
    ILI9341_WriteString(60, 20, "Temperature", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_FillRectangle(19, 69, 202, 182, ILI9341_BLACK);
    ILI9341_FillRectangle(60, 80, 120, 60, ILI9341_COLOR565(77,77,77));
    ILI9341_WriteString(93, 101, "UF200", Font_11x18, ILI9341_WHITE, ILI9341_COLOR565(77,77,77));
    ILI9341_FillRectangle(60, 170, 120, 60, ILI9341_COLOR565(77,77,77));
    ILI9341_WriteString(93, 191, "SHT30", Font_11x18, ILI9341_WHITE, ILI9341_COLOR565(77,77,77));
    Dispaly_EXIT_BUTTON();
}

/*------------------------------------------------------------------------------
Function name:  Dispaly_UF200_MENU
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function displays the UF200 menu on the display.
-------------------------------------------------------------------------------*/
void Dispaly_UF200_MENU() {
    ILI9341_FillRectangle(19, 69, 202, 182, ILI9341_BLACK);
    ILI9341_FillRectangle(59, 119, 122, 72, ILI9341_WHITE);
    ILI9341_FillRectangle(60, 120, 120, 70, ILI9341_BLACK);
    ILI9341_WriteString(93, 124, "UF200", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_FillRectangle(60, 146, 120, 1, ILI9341_WHITE);
}

/*------------------------------------------------------------------------------
Function name:  Update_Dispaly_UF200_MENU
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function updates the UF200 menu on the display.
-------------------------------------------------------------------------------*/
void Update_Dispaly_UF200_MENU(){
    format_temp_value(temp_uf200, "#C", temp_Data_String[TEMP_UF200_STRING], 16);
    ILI9341_WriteString(70, 160, temp_Data_String[TEMP_UF200_STRING], Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
}

/*------------------------------------------------------------------------------
Function name:  Dispaly_SHT30_MENU
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function displays the SHT30 menu on the display.
-------------------------------------------------------------------------------*/
void Dispaly_SHT30_MENU() {
    ILI9341_FillRectangle(19, 69, 202, 182, ILI9341_BLACK);
    ILI9341_FillRectangle(59, 119, 122, 99, ILI9341_WHITE);
    ILI9341_FillRectangle(60, 120, 120, 97, ILI9341_BLACK);
    ILI9341_FillRectangle(60, 183, 120, 1, ILI9341_WHITE);
    ILI9341_WriteString(93, 124, "SHT30", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_FillRectangle(60, 146, 120, 1, ILI9341_WHITE);

}

/*------------------------------------------------------------------------------
Function name:  Update_Dispaly_SHT30_MENU
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function updates the SHT30 menu on the display.
-------------------------------------------------------------------------------*/
void Update_Dispaly_SHT30_MENU(){
    format_temp_value(temp_sht30a, "#C", temp_Data_String[TEMP_SHT30_STRING], 16);
    format_temp_value(humidity_sht30a, "%", temp_Data_String[HUMITY_SHT30_STRING], 16);
    ILI9341_WriteString(70, 156, temp_Data_String[TEMP_SHT30_STRING], Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_WriteString(70, 190, temp_Data_String[HUMITY_SHT30_STRING], Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
}

/*------------------------------------------------------------------------------
Function name:  Dispaly_VOLTAGE_MENU
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function displays the VOLTAGE menu on the display.
-------------------------------------------------------------------------------*/
void Dispaly_VOLTAGE_MENU() {
    ILI9341_FillRectangle(20, 10, 220, 40, ILI9341_BLACK);
    ILI9341_WriteString(81, 20, "VOLTAGE", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_FillRectangle(19, 69, 202, 182, ILI9341_BLACK);
    ILI9341_FillRectangle(20, 100, 189, 76, ILI9341_WHITE);
    ILI9341_FillRectangle(21, 101, 187, 74, ILI9341_BLACK);
    ILI9341_FillRectangle(21, 126, 187, 1, ILI9341_WHITE);
    ILI9341_FillRectangle(21, 151, 187, 1, ILI9341_WHITE);
    ILI9341_WriteString(25, 105, "UF200", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_WriteString(25, 131, "LT1377", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_WriteString(25, 155, "BAT/3V3", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_FillRectangle(107, 100, 1, 76, ILI9341_WHITE);
    Dispaly_EXIT_BUTTON();
}

/*------------------------------------------------------------------------------
Function name:  Update_Dispaly_VOLTAGE_MENU
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function updates the VOLTAGE menu on the display.
-------------------------------------------------------------------------------*/
void Update_Dispaly_VOLTAGE_MENU(){
    format_voltage_value(volt_uf200, "V", voltage_Data_String[ADC_UF200], 16);
    format_voltage_value(volt_supply_uf200, "V", voltage_Data_String[ADC_SUPPLY_UF200], 16);
    format_voltage_value(volt_vbat, "V", voltage_Data_String[ADC_VBAT], 16);

    ILI9341_WriteString(113, 105, voltage_Data_String[ADC_UF200], Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_WriteString(113, 131, voltage_Data_String[ADC_SUPPLY_UF200], Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_WriteString(113, 155, voltage_Data_String[ADC_VBAT], Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
}

/*------------------------------------------------------------------------------
Function name:  Dispaly_SETTINGS_MENU
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function displays the SETTINGS menu on the display.
-------------------------------------------------------------------------------*/
void Dispaly_SETTINGS_MENU() {
    ILI9341_FillRectangle(20, 10, 220, 40, ILI9341_BLACK);
    ILI9341_WriteString(76, 20, "SETTINGS", Font_11x18, ILI9341_WHITE, ILI9341_BLACK);
    ILI9341_FillRectangle(19, 69, 202, 182, ILI9341_BLACK);
    ILI9341_FillRectangle(60, 170, 120, 60, ILI9341_COLOR565(77,77,77));
    ILI9341_WriteString(82, 191, "Standby", Font_11x18, ILI9341_WHITE, ILI9341_COLOR565(77,77,77));

    if (STATE_LT1377 == 1) {
         ILI9341_FillRectangle(60, 80, 120, 60, ILI9341_GREEN);
         ILI9341_WriteString(93, 90, "UF200", Font_11x18, ILI9341_BLACK, ILI9341_GREEN);
         ILI9341_WriteString(109, 110, "ON", Font_11x18, ILI9341_BLACK, ILI9341_GREEN);

     } else {
         ILI9341_FillRectangle(60, 80, 120, 60, ILI9341_RED);
         ILI9341_WriteString(93, 90, "UF200", Font_11x18, ILI9341_BLACK, ILI9341_RED);
         ILI9341_WriteString(104, 110, "OFF", Font_11x18, ILI9341_BLACK, ILI9341_RED);

     }
    Dispaly_EXIT_BUTTON();
}

/*------------------------------------------------------------------------------
Function name:  TOGGLE_LT1377
Input:          void
Output:         void
Author:         SPY
Date:           21.11.2024
Version:        1.0
Function description:
This function toggles the LT1377 on/off and updates the display.
-------------------------------------------------------------------------------*/
void TOGGLE_LT1377() {
    HAL_GPIO_TogglePin(LT1377_ON_OFF_GPIO_Port, LT1377_ON_OFF_Pin);
    HAL_Delay(1);
    if (STATE_LT1377 == 1) {
         ILI9341_FillRectangle(60, 80, 120, 60, ILI9341_GREEN);
         ILI9341_WriteString(93, 90, "UF200", Font_11x18, ILI9341_BLACK, ILI9341_GREEN);
         ILI9341_WriteString(109, 110, "ON", Font_11x18, ILI9341_BLACK, ILI9341_GREEN);

     } else {
         ILI9341_FillRectangle(60, 80, 120, 60, ILI9341_RED);
         ILI9341_WriteString(93, 90, "UF200", Font_11x18, ILI9341_BLACK, ILI9341_RED);
         ILI9341_WriteString(104, 110, "OFF", Font_11x18, ILI9341_BLACK, ILI9341_RED);

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

#ifdef  USE_FULL_ASSERT
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
