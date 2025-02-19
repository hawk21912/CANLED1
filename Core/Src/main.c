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

uint8_t               CANmsgReady;
CAN_RxHeaderTypeDef   RxHeader;
uint8_t               RxData[8];

typedef struct 
{
   uint8_t Selection;
   uint8_t Enable;
   uint8_t Function;
   uint8_t Shift;
   uint8_t Index; 
   uint8_t R;
   uint8_t G;
   uint8_t B;
   uint8_t Delay;   
   uint8_t Mult;

}ArrayConfigTypedef;
 ArrayConfigTypedef ARY;


#define ArrayConfig 0x0AFF0001
/*  Start bit ,  Signals   , len, byte 
 *  0 , Array Selection    , 3  , 1
 *  1 , Enable array       , 1  , 1
 *  2 , function           , 2  , 1 // demo, manual, reactive
 *  6 , shfit/sine         , 2  , 1        
 *  8 , array index        , 8  , 2
 *  16, Red value          , 8  , 3
 *  24, green value        , 8  , 4
 *  32, blue value         , 8  , 5
 *  40, delay              , 8  , 6 
 *  48, delay multiplier   , 4  , 7 delay = delay*multiplier only used for shift/sin settings
 *  
 *    
 * 
 */ 
typedef enum
{
  DEMO,
  MANUAL,
  REACTIVE
} FunctionStates;
#define ALL_ARRAYS 7


typedef struct 
{

  uint32_t time; 
  uint8_t  info; 
  uint8_t  replay;
  uint16_t match;
  uint8_t  msgRec;

}FRC_HEARTBBEAT_TypeDef;
 FRC_HEARTBBEAT_TypeDef FRC;
#define FRC_HEARTBBEAT 0x01011840
/*  Start bit ,  Signals    , len, byte 
 *  0 , Time of day hours  , 5 , 1
 *  5 , time of dat minutes, 6 , 1-2
 *  11, time of dat seconds, 6,  2-3
 *  17, time of day day    , 5 , 3
 *  22, time of day month  , 4 , 3-4
 *  26, time of day year   , 6 , 4
 *  32, tournament type    , 3 , 5 
 *  35, system watchdog    , 1 , 5  
 *  36, test mode          , 1 , 5
 *  37, Autonomus Mode     , 1 , 5 
 *  38, Enabled            , 1 , 5
 *  39, Red alliance       , 1 , 5
 *  40, Replay number      , 6 , 6
 *  46, match number       , 10, 6-7
 *  56, match time         , 8 , 8
 * 
 * vars
 *  uint32_t time   = bytes[1:4]
 *  uint8_t  info   = bytes[5:5]
 *  uint8_t  replay = bytes[6:6] //no way we care if there are more than 255 matches 
 *  uint16_t match  = bytes[7:8]
 */


#define WS2812_PWM_ONE_PER   66
#define WS2812_PWM_ZERO_PER  33

#define WS2812_PWM_ONE   WS2812_PWM_ONE_PER*TIM1->ARR/100
#define WS2812_PWM_ZERO  WS2812_PWM_ZERO_PER*TIM1->ARR/100



 // variables for LED
#define numArrays 2
#define numLEDs 25
 typedef struct{

  uint8_t numLED;// = numLEDs;
  uint8_t *LEDData;
  uint32_t *PWMdata;
  TIM_HandleTypeDef *htim;
  uint32_t Channel;
  uint8_t PWM_BUSY;

  }LEDArray;//*/
LEDArray Array[numArrays];

  
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void UpdateLEDs(LEDArray *LED);
void SetLED(LEDArray *LED,uint8_t pos, uint8_t R,uint8_t G, uint8_t B);
void initLEDArray(LEDArray *LED,uint8_t Length,TIM_HandleTypeDef *htim,uint32_t Channel);
void ShiftLED(LEDArray *LED,int sft);
void ProcessCANMessage();
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


  HAL_Delay(500);
  HAL_CAN_Start(&hcan);
  HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  
  //MASTER[1] 
  initLEDArray(&Array[0],numLEDs,&htim1,TIM_CHANNEL_1);
  initLEDArray(&Array[1],numLEDs,&htim2,TIM_CHANNEL_1);
  

    SetLED(&Array[1],0,50,0,0);
    UpdateLEDs(&Array[1]);

  ARY.Selection =1;
  while (1)
  {

    if(CANmsgReady == 1)
    {
      ProcessCANMessage();
    }


  for(int i = 0; i<numArrays;i++)
  {

    if(ARY.Selection == i || ARY.Selection == ALL_ARRAYS)
    {
  
      if(ARY.Enable == 1)
      {
        switch (ARY.Function)
        {
          case DEMO:
            /* code */
            break;
          
          case MANUAL:

            break;

          case REACTIVE:

            break;

          default:
            break;
      }

      }
      else 
      {
        
      }

      if(!Array[i].PWM_BUSY)
    {
      ShiftLED(&Array[i],-1);
      UpdateLEDs(&Array[i]);
   }
    }

    

  }
    HAL_Delay(34);
  
  if(!Array[1].PWM_BUSY)
   {
    ShiftLED(&Array[1],-1);
    UpdateLEDs(&Array[1]);
   }
  /* if(!Array[1].PWM_BUSY)
   {
    ShiftLED(&Array[1],-1);
    UpdateLEDs(&Array[1]);
   }*/
      

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
  Array[1].PWM_BUSY=0;

}

void initLEDArray(LEDArray *LED,uint8_t Length,TIM_HandleTypeDef *htim,uint32_t Channel)
{

  LED->numLED = Length;
  LED->LEDData = malloc(LED->numLED*3*sizeof(uint8_t));
  memset(LED->LEDData,0,LED->numLED*3*(sizeof(uint8_t)));
  LED->PWMdata = malloc(LED->numLED*24*sizeof(uint32_t));
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

void UpdateLEDs(LEDArray *LED)
{

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

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
 HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData);
 HAL_GPIO_TogglePin(LD3_GPIO_Port,LD3_Pin);  
 CANmsgReady = 1;
}

void ProcessCANMessage()
{

switch (RxHeader.ExtId)
{
case FRC_HEARTBBEAT:

  FRC.time   = (RxData[3]<<24) + (RxData[2]<<16) + (RxData[1]<<8) + (RxData[0]);
  FRC.info   = RxData[4];
  FRC.replay = RxData[5];
  FRC.match  = (RxData[7]<<8)  + RxData[7];

  /* code */
  break;

case ArrayConfig:

  ARY.Selection = (RxData[0] & 0b00000111)     ;
  ARY.Enable    = (RxData[0] & 0b00001000) >> 4;
  ARY.Function  = (RxData[0] & 0b00110000) >> 5;
  ARY.Shift     = (RxData[0] & 0b11000000) >> 6; 
  ARY.Index     = RxData[1];
  ARY.R         = RxData[2];
  ARY.G         = RxData[3];
  ARY.B         = RxData[4];
  ARY.Delay     = RxData[5];
  ARY.Mult      = RxData[6] & 0b00001111; 
  
  break;

default:
  break;


}

CANmsgReady = 0;
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
