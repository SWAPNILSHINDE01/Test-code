/* USER CODE BEGIN Header */
/**
*
*  Created on: Apr 29, 2026
*      Author: SwapnilShinde

  ******************************************************************************
  * @file           : main.c
  * @brief          : DestnoMeter - Optical Density Measurement System
  * @author         : SwapnilShinde
  *
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sh1107.h"
#include "tsl2591.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "adc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc3;

I2C_HandleTypeDef hi2c3;

RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static uint8_t current_data[5];
static uint8_t history_1[5];
static uint8_t history_2[5];
volatile uint8_t calibration_button_pressed = 0;
volatile uint8_t power_button_pressed = 0;
volatile uint8_t read_button_pressed = 0;
uint8_t one_time_calibration_flag = 1;
uint32_t last_interrupt_time = 0;
uint8_t restart_counter= 0;
volatile uint8_t charging_state_Pin_pressed = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C3_Init(void);
static void MX_ADC3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void UART_Log(char *msg) {
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

/**
 * @brief Helper to shift history and update current digits for display
 */
void Update_History(float lux) {
    // 1. Shift History Arrays (Full 5 bytes)
    memcpy(history_2, history_1, 5);
    memcpy(history_1, current_data, 5);

    // 2. Format new data for the custom display logic
    // Example: If LUX is 123.45, display might need specific digit mapping
    int32_t lux_whole = (int32_t)lux;
    int32_t lux_decimal = (int32_t)((lux - lux_whole) * 100);

    current_data[0] = (uint8_t)(lux_whole % 10);
    current_data[1] = (uint8_t)(lux_decimal / 10);
    current_data[2] = (uint8_t)(lux_decimal % 10);
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
  MX_USART2_UART_Init();
  MX_I2C3_Init();
  MX_ADC3_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE BEGIN 2 */

    // 3. Check if we just woke up from Standby
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
    {
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
//        enable battery chraging
        HAL_GPIO_WritePin(Charging_Enable_GPIO_Port, Charging_Enable_Pin, GPIO_PIN_SET);   // HIGH
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);

        UART_Log("System woke up from Standby. Peripherals re-initialized.\r\n");
    }
    else
    {
        UART_Log("Cold boot / Power-on reset.\r\n");
    }

   char uart_buf[128];
   uint32_t last_main_Tick = 0;


   // Verify connection to the display before trying to configure it
       if (HAL_I2C_IsDeviceReady(&hi2c3, SH1107_I2C_ADDR, 3, 50) != HAL_OK)
       {
    	   UART_Log("Display are not detected!\r\n");
           return HAL_ERROR; // Physical wires are missing or disconnected!
       }

       if (HAL_I2C_IsDeviceReady(&hi2c3, TSL2591_ADDR, 3, 50) != HAL_OK)
       {
    	   UART_Log("Sensor are not detected!\r\n");
           return HAL_ERROR; // Physical wires are missing or disconnected!
       }

   /* --- TSL2591 Sensor Init --- */
      ((TSL2591_Init(&hi2c3) == HAL_OK)?
        UART_Log("--- TSL2591 Sensor Initialized! ---\r\n"):
        UART_Log("--- TSL2591 Sensor Init Failed! ---\r\n"));

   /* --- SH1107 Display Init --- */
     if (sh1107_Init() == HAL_OK) // Most sh1107 drivers return 0 for success
     {

       UART_Log("--- SH1107 Display Initialized ---\r\n");
       sh1107_Fill(Black);
       sh1107_UpdateScreen(); // Clear the physical glass
       HAL_Delay(50);
       sh1107_DisplayFirstScreen(); // Logo/Splash
       sh1107_UpdateScreen();
       HAL_Delay(1500);
       sh1107_Fill(Black);
       sh1107_UpdateScreen(); // Clear the physical glass
       HAL_Delay(50);
	   sh1107_DisplayCallibrationScreen();
	   HAL_Delay(50);
	   sh1107_UpdateScreen(); // update the physical display
     }
     else
     {
         /* --- FAILURE CHECKPOINT --- */
         UART_Log("--- SH1107 Display Init Failed! Bus or Device Error ---\r\n");
     }

     UART_Log("Initialize Complete.\r\n");

     HAL_Delay(500);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  /*
	   * Check wheather user pressed a calibration button or not.
	   */

	  uint32_t current_main_Tick = HAL_GetTick();


	  // Draw a battry symbol with fill battery %  at the upper left side.
	      int  battery_percent = percentage_return();
//	      int  battery_percent = 100;
	      // TO off charging battaery

	      if(charging_state_Pin_pressed)
	      {
	      if (battery_percent > 99)
	      {

	      HAL_GPIO_WritePin(Charging_Enable_GPIO_Port, Charging_Enable_Pin, GPIO_PIN_RESET);
	      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
//	      HAL_Delay(50);
//	      HAL_GPIO_WritePin(Charging_Enable_GPIO_Port, Charging_Enable_Pin, GPIO_PIN_SET);
//	      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
	      }

	      else {
	    	  if(HAL_GPIO_ReadPin(GPIOA, charging_state_Pin))

	    	  	  {
			  UART_Log("Device is Charging.\r\n");
		      HAL_GPIO_WritePin(Charging_Enable_GPIO_Port, Charging_Enable_Pin, GPIO_PIN_SET);
	//	      HAL_Delay(50);
		      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
		       HAL_Delay(50);
	    	  	  }
	    	  else{
				  UART_Log("Charger is not connected.\r\n");
			      HAL_GPIO_WritePin(Charging_Enable_GPIO_Port, Charging_Enable_Pin, GPIO_PIN_RESET);
		//	      HAL_Delay(50);
			      HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
			       HAL_Delay(50);
	    	  }
	      	  }
	      }
	      sh1107_DrawProgressBar(battery_percent);
	      sh1107_UpdateScreen();


	  if((current_main_Tick - last_main_Tick) > (30 * 1000) )   //30*1000 mSec
	  {
		  UART_Log("STOP MODE ON.\r\n");
//		  last_main_Tick =  current_main_Tick;
    	  // 2. Enable Power Clock
    	  __HAL_RCC_PWR_CLK_ENABLE();

    	  /* 3.  IMPORTANT: Clear the WAKEUP flag AND the STANDBY flag
    	        need to clear both to reset the internal latch
    	  */
    	  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    	  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);

    	  /*
    	     4. CRITICAL: Clear any pending EXTI interrupts
    	    * This prevents a legacy button press flag from waking the MCU up instantly.
    	   */
    	  __HAL_GPIO_EXTI_CLEAR_IT(read_Pin);

    	  /* 5. CRITICAL: Disable the 1ms SysTick interrupt
    	     Otherwise, SysTick will wake up the MCU in less than 1 millisecond.
    	  */
//    	  HAL_SuspendTick();
    	  HAL_Delay(50);
    	  /* Set the RTC Auto-Wakeup Timer to trigger an interrupt every 5 seconds */
    	  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 5, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);

    	  if(restart_counter > 5)
    	  {
    		  UART_Log("Shut Down mode.\r\n");
    		  power_button_pressed = 1;
    		  sh1107_DisplayCustomData(current_data);
    	  }


    	  /*
    	   * we only sleep in tls sensor because battery symbol we will be required to pop up in some duration for identify the device is sleep mode.
    	   *
    	   */

    	  if (TSL2591_Sleep(&hi2c3) != HAL_OK)
    	      {
    		  UART_Log("TLS2591 sensor are not in sleep mode.\r\n");// Handle I2C error if the sensor didn't acknowledge
    	      }
    	  else {
    		  UART_Log("TLS2591 sensor in a sleep mode.\r\n");// Handle I2C error if the sensor didn't acknowledge
    	  }

    	  /* Drop into Stop Mode. The CPU turns OFF right here. */
    	  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);


    	  // Deactivate the wakeup timer so it doesn't fire continuously while we work
    	  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

    	  /* 6. CRITICAL: Restore system clocks back to full speed (PLL, HSE, etc.) */
    	  SystemClock_Config();

    	  /* 7. CRITICAL: Turn the 1ms SysTick interrupt back on for
    	   *  HAL_Delay/Timeouts
    	   */
    	  HAL_ResumeTick();


    	  restart_counter++ ;


    	  /* 8. Re-initialize or re-verify hardware peripherals if required */
    	  UART_Log("Woke up from Stop Mode! Clock system restored.\r\n");

    	  // 7. Re-initialize the I2C Bus to resume communications
    	      MX_I2C3_Init();

    	      // 8. Wake the TSL2591 back up
    	      /* --- TSL2591 Sensor Init --- */
    	         ((TSL2591_Wakeup(&hi2c3) == HAL_OK)?
    	           UART_Log("--- TSL2591 Sensor Wakeup! ---\r\n"):
    	           UART_Log("--- TSL2591 Sensor wakeup failed! ---\r\n"));

    	  HAL_Delay(200);
  }

      /* --- Manual Re-Calibration Trigger--- */
      if (calibration_button_pressed)
      {
          UART_Log("Manual Re-calibration button pressed....\r\n");
		  sh1107_Fill(Black);
		  sh1107_UpdateScreen();
		  HAL_Delay(50);
		  sh1107_DisplayCallibrationScreen();
		  sh1107_UpdateScreen(); // update the physical display
		  last_main_Tick = HAL_GetTick();
		  __HAL_GPIO_EXTI_CLEAR_IT(calibration_Pin);
		  one_time_calibration_flag = 1;
          calibration_button_pressed = 0;
          HAL_NVIC_EnableIRQ(EXTI1_IRQn);
          HAL_Delay(50);
	  }


	  // battery_percent==0  important when battery % is 0 but controller is connected to a  2.7V Battery
      if(power_button_pressed  || battery_percent == 0 )
      {
    	  UART_Log("STAND BY MODE ON.\r\n");
    	  ((power_button_pressed == 1)?
    	  UART_Log("Power Button trigger\r\n") :
    	  UART_Log(" Due to Battery drop.\r\n"));
    	  UART_Log("Power OFF...\r\n");
    	  sh1107_Fill(Black);
    	  sh1107_UpdateScreen();
    	  HAL_Delay(50);
    	  sh1107_EndDisplay();
    	  sh1107_UpdateScreen();
    	  HAL_Delay(1000);
    	  power_button_pressed = 0;
    	  sh1107_Fill(Black);
    	  sh1107_UpdateScreen();
    	  HAL_Delay(50);

    	  //more power consumtion i2c sensor are in sleep mode.
    	  if (TSL2591_Sleep(&hi2c3) != HAL_OK)
    	      {
    		  UART_Log("TLS2591 sensor are not in sleep mode.\r\n");// Handle I2C error if the sensor didn't acknowledge
    	      }
    	  else {
    		  UART_Log("TLS2591 sensor in a sleep mode.\r\n");// Handle I2C error if the sensor didn't acknowledge
    	  }
    	  if (sh1107_Sleep() != HAL_OK)
    	  {
    		  UART_Log("Display are not in sleep mode.\r\n");// Handle I2C error if the sensor didn't acknowledge
    	  }
    	  else
    	  {
    	    UART_Log("Display in a sleep mode.\r\n");// Handle I2C error if the sensor didn't acknowledge
    	   }
    	  if (HAL_I2C_DeInit(&hi2c3) == HAL_OK) {
    		  UART_Log("I2C_off...\r\n");
    	  }

    	  if(HAL_ADC_Stop(&hadc3)==HAL_OK)
    	  {
    		  UART_Log("ADC_off...\r\n");
    	  }/* Deinitialize GPIO */
    	  HAL_GPIO_DeInit(GPIOA,

    	                  calibration_Pin   |
    	                  read_Pin          |
    	                  power_Pin);
//    	  __HAL_RCC_GPIOA_CLK_DISABLE();

    	  /* Disable EXTI interrupt */
    	  GPIO_InitTypeDef GPIO_InitStruct = {0};
    	  GPIO_InitStruct.Pin  = charging_state_Pin ;
    	  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    	  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    	  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

//    	  HAL_NVIC_DisableIRQ(EXTI1_IRQn);
//    	  HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
//    	  HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);



    	  // 2. Enable Power Clock
    	  __HAL_RCC_PWR_CLK_ENABLE();

    	  // 1. Disable the pin to prevent electrical conflict

    	  // 3. Disable the wakeup pin configuration register first
    	            HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
    	      	  // 3. IMPORTANT: Clear the WAKEUP flag AND the STANDBY flag
    	      	  // need to clear both to reset the internal latch
    	      	  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    	      	  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
//    	    	  __HAL_GPIO_EXTI_CLEAR_IT(power_Pin);
//    	    	  __HAL_GPIO_EXTI_CLEAR_IT(charging_state_Pin);

    	            // 5. Enable Wakeup Pin 2 (PC13) in native RISING mode
    	            // On STM32F3, this function configures WKUP2 natively for a rising edge.
    	            HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
    	            HAL_Delay(500 );
    	  HAL_SuspendTick();
    	  HAL_PWR_EnterSTANDBYMode();
    	  HAL_ResumeTick();
      }

      if(read_button_pressed)
      {
    	  UART_Log("|| READING SENSOR DATA || \r\n");
    	  float i0_reffernce;
         if (one_time_calibration_flag == 1)
         {
        	 i0_reffernce = TSL2591_Calibrate(&hi2c3);
        	 one_time_calibration_flag =0;

         }
      /* --- Data Acquisition --- */
       uint16_t full, ir;
	  /* 1. Read Sensor */
       TSL2591_ReadData(&hi2c3, &full, &ir);
	      //Calculate a vary basic Lux estimate
       float lux = TSL2591_CalculateLux(full, ir);
       float od = 0.0f;
       if (lux < 0) {
           // SENSOR IS SATURATED

           sprintf(uart_buf, "RAW: %u | SENSOR SATURATED - LOWER GAIN!\r\n", full);
           UART_Log(uart_buf);
       }
       else
       {
       od = TSL2591_GetOpticalDensity(lux);
       }

       /* --- UI Update --- */
         Update_History(od);

         sh1107_Fill(Black);

         /* --- Logging --- */
         uint32_t lux_whole = (int)(lux);
         uint32_t lux_fractional_number = (int)((lux - (float)lux_whole) * 100.0f);

         uint32_t od_whole = (int)(od);
         uint32_t od_fractional_number = (int)((od - (float)od_whole) * 100.0f);

         uint32_t i0_reffernce_whole = (int)(i0_reffernce);
         uint32_t i0_fractional_number = (int)((i0_reffernce - (float)i0_reffernce_whole) * 100.0f);

         sprintf(uart_buf, "RAW: %u | LUX : %d.%02d | OD : %d.%02d  | i0 : %d.%d\r\n",
        		 full, lux_whole, lux_fractional_number,od_whole ,
        		 od_fractional_number,i0_reffernce_whole,i0_fractional_number);

          UART_Log(uart_buf);


	      // 5.2. Draw Current Main large vaule.
	      sh1107_DisplayCustomData(current_data);



	      // 5.4. Draw History (Smaller secondary displays)
	      sh1107_DisplayCustomData_1(history_1);
	      sh1107_DisplayCustomData_2(history_2);



	   	  // 5.5. Draw a line at X=32, from Y=0 to Y=127 */
	      // UI Separator Line
	      SH1107_DrawVerticalLine(43, 0, 127, White);


	    	  /*
	    	   *
	    	   */
	      sh1107_UpdateScreen(); // Push all drawings to display
	      read_button_pressed = 0;
	      HAL_Delay(50);
	      last_main_Tick = HAL_GetTick();
    	  __HAL_GPIO_EXTI_CLEAR_IT(read_Pin);
    	  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
	      }

	      /* 6. currently its a polling based screen refreshment
	       *  if you want after change to a interrupt based.
	       */
	      HAL_Delay(1000);

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART2
                              |RCC_PERIPHCLK_I2C3|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c3ClockSelection = RCC_I2C3CLKSOURCE_HSI;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */

  /** Common config
  */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DMAContinuousRequests = DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc3.Init.LowPowerAutoWait = DISABLE;
  hadc3.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc3, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.SamplingTime = ADC_SAMPLETIME_601CYCLES_5;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */
  if (HAL_ADCEx_Calibration_Start(&hadc3, ADC_SINGLE_ENDED) != HAL_OK)
  {
      Error_Handler();
  }
  /* USER CODE END ADC3_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00201D2B;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 38400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 38400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Charging_Enable_GPIO_Port, Charging_Enable_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : charging_state_Pin calibration_Pin read_Pin power_Pin */
  GPIO_InitStruct.Pin = charging_state_Pin|calibration_Pin|read_Pin|power_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Charging_Enable_Pin */
  GPIO_InitStruct.Pin = Charging_Enable_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Charging_Enable_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* Enable and set EXTI Line [15:10] Interrupt to wake up the MCU */
      HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
      HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	uint32_t current_time = HAL_GetTick(); // Get time in milliseconds

    if (GPIO_Pin == calibration_Pin)
    {
    	HAL_NVIC_DisableIRQ(EXTI1_IRQn);
    	 if ((current_time - last_interrupt_time) > 200)
    	 {
    		 UART_Log("calibration_Pin2\r\n");
    		 calibration_button_pressed = 1;
    		 last_interrupt_time = current_time; // Update the last trigger time
    	 }
    }

    if (GPIO_Pin == charging_state_Pin)
    {
    	HAL_NVIC_DisableIRQ(EXTI0_IRQn);
    	 if ((current_time - last_interrupt_time) > 200)
    	 {
    		 UART_Log("charging_state_Pin\r\n");
    		 charging_state_Pin_pressed = 1;
//    	        charging_event = 1;
    		 last_interrupt_time = current_time; // Update the last trigger time
    	 }
    }

    else if (GPIO_Pin == power_Pin)
    {
    	HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
   	 if ((current_time - last_interrupt_time) > 200)
   	 {
   		 UART_Log("power_Pin_2\r\n");
   		 power_button_pressed = 1;
   		 last_interrupt_time = current_time; // Update the last trigger time
   	 }
    }

   	 else if (GPIO_Pin == read_Pin)
   	 {
    	HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
      	 if ((current_time - last_interrupt_time) > 200)
      	 {
      		UART_Log("read_Pin2\r\n");
      		read_button_pressed = 1;
      		last_interrupt_time = current_time; // Update the last trigger time
      	 }
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
  /* User can add his own implementation to re port the HAL error return state */
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
