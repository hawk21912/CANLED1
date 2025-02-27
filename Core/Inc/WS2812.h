

#ifndef __WS2812__
#define __WS2812__

#define WS2812_PWM_ONE_PER   66
#define WS2812_PWM_ZERO_PER  33

#define WS2812_PWM_ONE   WS2812_PWM_ONE_PER*TIM1->ARR/100
#define WS2812_PWM_ZERO  WS2812_PWM_ZERO_PER*TIM1->ARR/100

#define numArrays 2
#define numLEDs 180


#include "main.h"
#include "can.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

#include "stdlib.h"
#include "string.h"



 typedef struct{

  uint8_t numLED;// = numLEDs;
  uint8_t LEDData[numLEDs*3];
  uint8_t PWMdata[numLEDs*3*8 + 50];
  uint8_t Enabled;
  TIM_HandleTypeDef *htim;
  uint32_t Channel;
  uint8_t PWM_BUSY;

  }LEDArray;//*/

typedef struct 
{
   uint8_t Selection;
   uint8_t Enable;
   uint8_t Function;
   int8_t Shift;
   uint8_t Index; 
   uint8_t R;
   uint8_t G;
   uint8_t B;
   uint8_t Delay;   
   uint8_t Mult;

}ArrayConfigTypedef;


extern LEDArray Array[numArrays];
extern ArrayConfigTypedef CFG;

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

void initLEDArray(LEDArray *LED,uint8_t Length,TIM_HandleTypeDef *htim,uint32_t Channel);
void UpdateLEDs(LEDArray *LED);
void SetLED(LEDArray *LED,uint8_t pos, uint8_t R,uint8_t G, uint8_t B);
void ShiftLED(LEDArray *LED,int sft);
int HSVtoRGB(int H,int S, int V);
void Demo1(LEDArray *LED,uint8_t PreviousFunciton);
void Demo2(LEDArray *LED);

#endif