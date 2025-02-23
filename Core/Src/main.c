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
 ArrayConfigTypedef CFG;


#define ArrayConfig 0x0AFF0001
/*  Start bit ,  Signals   , len, byte 
 *  0 , Array Selection    , 3  , 1
 *  1 , Enable array       , 1  , 1
 *  2 , function           , 2  , 1 // demo1,demo2, manual, reactive
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
  DEMO1,
  DEMO2,
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
#define FRC_HEARTBBEAT 0x01011840  // for waveforms use 040 11840 not sure why I cant just enter the full ext id but go off ig
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
#define numArrays 1
#define numLEDs 120

// so far max is 367  numArrays*numLeds I NEEED MORE RAM I wouldnt do more than 300 
 typedef struct{

  uint8_t numLED;// = numLEDs;
  uint8_t LEDData[numLEDs*3];
  uint8_t PWMdata[numLEDs*3*8 + 50];

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
int HSVtoRGB(int H,int S, int V);
void Demo1(LEDArray *LED,uint8_t PreviousFunciton);
void Demo2(LEDArray *LED);
void FRCHeartBeatReactive(LEDArray *LED);
void SineWave(LEDArray *LED,  uint8_t col, float A, float offset );
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
  MX_TIM17_Init();
  /* USER CODE BEGIN 2 */
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  HAL_CAN_Start(&hcan);
  HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

  //initLEDArray(&Array[0],numLEDs,&htim1,TIM_CHANNEL_1);
  initLEDArray(&Array[0],numLEDs,&htim2,TIM_CHANNEL_1);
  
  CFG.Selection =0;
  CFG.Enable =1;
  CFG.Function = REACTIVE;

  FRC.match = 0x0;//FF;

  int FunctionLast =-1;

  while (1)
  {

 
  if(CANmsgReady == 1)
    {
      ProcessCANMessage();
    }


  for(int i = 0; i<numArrays;i++)
  {

    if(CFG.Selection == i || CFG.Selection == ALL_ARRAYS)
    {
  
      if(CFG.Enable == 1)
      {
        switch (CFG.Function)
        {
          case DEMO1:           
            Demo1(&Array[i],FunctionLast);
            break;
          
          case DEMO2:                           
            Demo2(&Array[i]);            
            break;

          case MANUAL:
            SetLED(&Array[i],CFG.Index,CFG.R,CFG.G,CFG.B);
            break;

          case REACTIVE:
            FRCHeartBeatReactive(&Array[i]);
            break;

          default:
            break;
      }

        FunctionLast = CFG.Function;
      }
      else 
      {
        //Turn off All LEDs when the array isnt enabled by setting LED Data to 0
        memset(Array[i].LEDData, 0, sizeof(Array[i].LEDData));
      }

      if(!Array[i].PWM_BUSY)
      {
      UpdateLEDs(&Array[i]);
      }
    }

  }
    HAL_Delay(17);
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
  Array[0].PWM_BUSY=0;

}

void initLEDArray(LEDArray *LED,uint8_t Length,TIM_HandleTypeDef *htim,uint32_t Channel)
{

  LED->numLED = Length;
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

    for(int k=7;k>=0;k--){

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

int HSVtoRGB(int H,int S, int V)
{
    const int factor = 255;

    int C = S*V/factor;
    int m = (V-C);
    int hp = (factor*H/60);
    int X = (C*(factor-abs(hp%(2*factor) - factor)))/factor;
    hp/=factor;

    int r=0;
    int g=0;
    int b=0;

    if( 0<= hp && hp < 1)
    {
        r=C;
        g=X;
    }
    else if( 1<= hp && hp < 2)
    {
        r=X;
        g=C;
    }
    else if( 2<= hp && hp < 3)
    {
        g=C;
        b=X;
    }
    else if( 3<= hp && hp < 4)
    {
        g=C;
        b=X;
    }
    else if( 4<= hp && hp < 5)
    {
        r=X;
        b=C;
    }
    else if( 5<= hp && hp < 6)
    {
        r=C;
        b=X;
    }

    r=((r+m));
    g=((g+m)); 
    b=((b+m));

    int rgb = ((0xFF & b)<<16) + ((0xFF&g)<<8) + r;
 
    return(rgb);
}

void Demo1(LEDArray *LED,uint8_t PreviousFunciton)
{
  
  int AngleDiff = 360/numLEDs;
  uint32_t rgb =0;

   if(PreviousFunciton != DEMO1)
    {
      for(int i = 0; i < LED->numLED;i++)
      {
      rgb = HSVtoRGB(i*AngleDiff,255,30);
      SetLED(LED,i, rgb & 0XFF, (rgb & 0xFF00)>>8, (rgb & 0xFF0000)>>16 );
      }

    }
    else
    {
      ShiftLED(LED,-1);
    }


}

void Demo2(LEDArray *LED)
{
  static uint16_t demo2angle=0;
  uint32_t rgb=0;

  if(demo2angle >=360)
  {
    demo2angle=0;
  }

  rgb= HSVtoRGB(demo2angle,255,30);

  for(int i = 0;i<numLEDs;i++){
     SetLED(LED,i, rgb & 0XFF, (rgb & 0xFF00)>>8, (rgb & 0xFF0000)>>16 );
  }

  demo2angle++;
}

void SineWave(LEDArray *LED,  uint8_t col, float A, float offset )
{


  for(float i =0; i< (float) LED->numLED;i++)
  {
    float res= offset + A*sin(2*3.14f*i/ ((float) LED->numLED));

    switch (col)
    {
    case 0://red
      SetLED(LED,i,(uint8_t)res,0,0);
      break;

    case 1://green
      SetLED(LED,i,0,(uint8_t)res,0);
      break;

    case 2://blue
      SetLED(LED,i,0,0,(uint8_t)res);
      break;

    default:
      break;
    }
  }

}

void FRCHeartBeatReactive(LEDArray *LED)
{

  static uint8_t PrevColor = -1;
  uint8_t color = (FRC.info & 0b10000000);

  if(PrevColor != color)
  {

    if(color == 0x80) //red alliance
    {
        SineWave(LED,0,50, 70);
    } 
    else if( color == 0) //blue alliance
    {
        SineWave(LED,2,50, 70);
    }
    
  } else {

    ShiftLED(LED,-1);
  }


  PrevColor = color;

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

  CFG.Selection = (RxData[0] & 0b00000111)     ;
  CFG.Enable    = (RxData[0] & 0b00001000) >> 4;
  CFG.Function  = (RxData[0] & 0b00110000) >> 5;
  CFG.Shift     = (RxData[0] & 0b11000000) >> 6; 
  CFG.Index     = RxData[1];
  CFG.R         = RxData[2];
  CFG.G         = RxData[3];
  CFG.B         = RxData[4];
  CFG.Delay     = RxData[5];
  CFG.Mult      = RxData[6] & 0b00001111; 
  
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
