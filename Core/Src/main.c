/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "can.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdlib.h"
#include "string.h"
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

/* USER CODE BEGIN PV */
 CAN_TxHeaderTypeDef   TxHeader;
  uint8_t               TxData[8];
  uint32_t              TxMailbox;

  
  const uint32_t WS2812_PWM_ONE_PER  = 66;
  const uint32_t WS2812_PWM_ZERO_PER = 33;

  uint32_t WS2812_PWM_ONE;  
  uint32_t WS2812_PWM_ZERO;
  

 // variables for LED

  #define numLEDs 5
 typedef struct{

    uint8_t numLED;// = numLEDs;
    uint8_t *LEDData;
    uint32_t *PWMdata;
    TIM_HandleTypeDef *htim;
    uint32_t Channel;
    uint8_t PWM_BUSY;

  }LEDArray;//*/
  LEDArray Array1;


  
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void UpdateLEDs(LEDArray *LED);
void SetLED(LEDArray *LED,uint8_t pos, uint8_t R,uint8_t G, uint8_t B);
void initLEDArray(LEDArray *LED,uint8_t Length,TIM_HandleTypeDef *htim,uint32_t Channel);
void ShiftLED(LEDArray *LED,int sft);
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
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_CAN_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.StdId = 0x446;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.DLC = 2;

  TxData[0] = 50;  
  TxData[1] = 0xAA;


  HAL_Delay(500);
  HAL_CAN_Start(&hcan);
  HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  WS2812_PWM_ONE  = WS2812_PWM_ONE_PER*TIM1->ARR/100;
  WS2812_PWM_ZERO = WS2812_PWM_ZERO_PER*TIM1->ARR/100;
 
  initLEDArray(&Array1,numLEDs,&htim2,TIM_CHANNEL_1);

  /*wd SetLED(&Array1,0,2,1,3);
    SetLED(&Array1,1,5,4,6);
    SetLED(&Array1,2,8,7,9);
    SetLED(&Array1,3,11,10,12);
    SetLED(&Array1,4,14,13,15);//*/

 SetLED(&Array1,0,50,0,0);
    SetLED(&Array1,1,0,50,0);
    SetLED(&Array1,2,0,0,50);
    SetLED(&Array1,3,50,50,50);
    SetLED(&Array1,4,0,0,0);//*/
    UpdateLEDs(&Array1);

    int shift = 1;
  while (1)
  {

    HAL_Delay(250);
  
   if(!Array1.PWM_BUSY)
   {
    /*SetLED(&Array1,0,255,0,0);
    SetLED(&Array1,1,0,50,0);
    SetLED(&Array1,2,0,0,50);
    SetLED(&Array1,3,50,50,50);
    SetLED(&Array1,4,0,0,0);*/
    ShiftLED(&Array1,1);
    UpdateLEDs(&Array1);
   }
      
  
    
    HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_TIM1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLK_HCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{

  HAL_TIM_PWM_Stop_DMA(&htim2, TIM_CHANNEL_1);
  Array1.PWM_BUSY=0;

}

void initLEDArray(LEDArray *LED,uint8_t Length,TIM_HandleTypeDef *htim,uint32_t Channel)
{

  LED->numLED = Length;
  LED->LEDData = malloc(Array1.numLED*3*sizeof(uint8_t));
  LED->PWMdata = malloc(Array1.numLED*24*sizeof(uint32_t));
  LED->htim = htim;
  LED->Channel = Channel;
  LED->PWM_BUSY = 0;

}

void ShiftLED(LEDArray *LED,int sft)
{
  sft*=3;
  uint8_t cpy[3*LED->numLED];
  
  memcpy(cpy,LED->LEDData,sizeof(cpy));
  
  for(int i =0; i < LED->numLED*3 ;i++)
  {
    if(i+sft >= 3*LED->numLED)
    {
      LED->LEDData[i] = cpy[i+sft-(3*LED->numLED)];
    } 
    else if (i+sft < 0)
    {
      LED->LEDData[i] = cpy[i+sft+(3*LED->numLED)];
    }
    else
    {
      LED->LEDData[i]= cpy[i+sft];
    }
  }

}

void SetLED(LEDArray *LED,uint8_t pos, uint8_t R,uint8_t G, uint8_t B)
{

  if( 0 <= pos && pos < LED->numLED)
  {    
    LED->LEDData[pos*3 ]= G ;
    LED->LEDData[pos*3 +1]= R;
    LED->LEDData[pos*3 +2]= B;

  }

}
void UpdateLEDs(LEDArray *LED){

  uint16_t bitState=0;
  uint16_t index=0;

  for(int i=0; i < 3*LED->numLED; i++)
  {  
    bitState = LED->LEDData[i];       

    for(int k=0;k<8;k++){

      if((bitState>> k) & 1)
      {
        LED->PWMdata[index] = WS2812_PWM_ONE;
      } 
      else
      {
        LED->PWMdata[index] = WS2812_PWM_ZERO;
      }
      index++;
    }    
  }

  for(int i = LED->numLED*24; i < LED->numLED*24 + 50; i++){
    LED->PWMdata[i]= 0;
    index++;
  }

  HAL_TIM_PWM_Start_DMA(LED->htim,LED->Channel,LED->PWMdata , (LED->numLED*24) +50);
  LED->PWM_BUSY=1;

}


CAN_RxHeaderTypeDef   RxHeader;
uint8_t               RxData[8];

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
 HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData);
 HAL_GPIO_TogglePin(LD3_GPIO_Port,LD3_Pin);
  
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
