

#include "WS2812.h"



LEDArray Array[numArrays];
ArrayConfigTypedef CFG;


/*
 * Function Name 
 *    Initialize LEDArray
 * 
 * Parameters
 *    LEDArray *LED           | The LEDArray Struct that is to be initalized
 *    uint8_t Length          | Length of LED strip 
 *    TIM_HandleTypeDef *htim | The STM32HAL timer handle that will be used for the channel ex &htim1
 *    uint32_t Channel        | the timer channel that is to be use. please use the HAL timer ex TIM_CHANNEL_1 
 *                              and do not use 1,2,3 etc
 * Description
 *  Initializes an LEDArray Struct.Struct needs to be initalized by either calling this 
 *  or just assinging the parameters manually. The code will more than likley hardfault if this
 *  has not been called before any LED funcitons are used. for timer setting you would need to check the .ioc 
 *  file for more information however in short TIM frequency= 800Khz or ARR = (APB / 800000) - 1 AND PSC = 0
 *  for DMA setting you want the channel ex TIM2_CH1 with the direction of Memory to perf, increment Memory address , 
 *  Data width for perf to be whatever the Timer used in my case it was a Word and the Memory Data width to be a byte
 * 
 * 
 */
void initLEDArray(LEDArray *LED,uint8_t Length,TIM_HandleTypeDef *htim, uint32_t Channel)
{

  LED->numLED = Length;
  LED->htim = htim;
  LED->Channel = Channel;
  LED->PWM_BUSY = 0;
  LED->Enabled = 1;

}


/*
 * Function Name 
 *    Shift LED Array 
 * 
 * Parameters
 *    LEDArray *LED | The LEDArray to be shifted
 *    int sft       | the shift amount 
 
 * Description
 *    Shifts entire Strip of LEDs by the amount specified by sft.
 *    Due to the logic of how this funcion is defined positive numbers
 *    will shift the array towards the first LED and negitive numbers 
 *    will shift towards the last LED. 
 * 
 */
void ShiftLED(LEDArray *LED,int sft)
{
  //multiplies by a factor of 3 since LEDData stores R,G,B for each in a different index so to shift by 1 you needs to shift by 3
  sft*=3; 

  //Creates a copy of the array that will be used to map to the shifted array
  uint8_t cpy[3*LED->numLED];
  memcpy(cpy,LED->LEDData,sizeof(cpy));
  
  //for All of LEDData
  for(int i =0; i < LED->numLED*3 ;i++)
  {
    //if shift overflows the array wrap around to begining 
    if(i+sft >= 3*LED->numLED)
    {
      LED->LEDData[i] = cpy[i+sft-(3*LED->numLED)];
    } 
    // if shift underflows array wrap around to end
    else if (i+sft < 0)
    {
      LED->LEDData[i] = cpy[i+sft+(3*LED->numLED)];
    }
    else
    {
      //if no overflow or underflow than shift accordingly
      LED->LEDData[i]= cpy[i+sft];
    }
  }

}


/***
 * Function Name
 *    SetLED
 * 
 * Parameters
 *  LEDArray *LED | LED array to be used 
 *  uint8_t pos   | what LED to be addressed 
 *  uint8_t R     | Red value to be used
 *  uint8_t G     | Green value ot be used
 *  uint8_t B     | Blue value to be used 
 * 
 * Description 
 * 
 *  map R,G,B to where is sits in LEDData 
 * for the WS2812 LEDs they follow the order of
 * GRB and not RGB so the change is applied here additionally 
 * position is multipled by 3 as the data for 
 * real LED1 ->LEDData[0],LEDData[1],LEDData[2]
 * This was done to save memory and potetnionally and avoid 
 * using a 2d array to save complexity (however it didnt by much)
 *  
*/
void SetLED(LEDArray *LED,uint8_t pos, uint8_t R,uint8_t G, uint8_t B)
{
  //if the position within the range of the array 
  if( 0 <= pos && pos < LED->numLED)
  {  
    //map the RGB data
    LED->LEDData[pos*3]  = G ;
    LED->LEDData[pos*3+1]= R;
    LED->LEDData[pos*3+2]= B;

  }

}


/**
 * Funciton name
 *   UpdateLEDs
 * 
 * Parameters
 *  LEDArray *LED | LED array to be updated
 * 
 * Description
 *  Phyically updates the color of the LEDS
 *  Takes the informaiton mapped from LEDdata expands
 *  each bit and encodes them into PWM duty cycles based on 
 *  either a logical bit 1 or logical bit 0. 
 * 
 *  the WS2812 LED requires a PWM of 800KHz or 1.25us period for each bit
 *  it reqires that
 *  logical 1 to be about 850ns high followed by 400ns low
 *  logical 0 to be about 400ns high followed by 850ns low
 * 
 *  for how the PWM is configured with DMA by feeding it an array of these values
 *  on every Timer Period or PMW high time + PWM low time it will update its duty 
 *  cycle to the next entry of the array 
 * 
 *  
 * 
 */
void UpdateLEDs(LEDArray *LED)
{

  uint16_t bitState=0; //used to break apart each byte into 8 bits
  uint16_t index=0;    //used to correctly assign the bit encodings to its proper spot in the array

  //for all of LEDData 
  for(int i=0; i < 3*LED->numLED; i++)
  {  

    //get the value of each byte
    bitState = LED->LEDData[i];       

    //for each byte counting in reverse order as the WS2812 wants data to be MSb to LSb
    for(int k=7;k>=0;k--)
    {  
      //if current bit = 1 assign PWMData entry to duty cycle for logical one
      if((bitState>> k) & 1)
      {
        LED->PWMdata[index] = WS2812_PWM_ONE;
      }
      // else (if logical 0) assign PWMdata entry to duty cycle for logical 0 
      else
      {
        LED->PWMdata[index] = WS2812_PWM_ZERO;
      }
      //increase index in PWMdata
      index++;
    }    
  }

  //pad and extra 50 bit times with 0 (not logical but acutal PWM duty of 0)to account for the WS2812 reset time
  for(int i = LED->numLED*24; i < LED->numLED*24 + 50; i++){
    LED->PWMdata[i]= 0;
    index++;
  }

  //start the DMA on the PWM timer and channel specified in the LED Array and set the PWM busy to 1 
  HAL_TIM_PWM_Start_DMA(LED->htim,LED->Channel,LED->PWMdata , (LED->numLED*24) +50);
  LED->PWM_BUSY=1;

}


/**
 * Funciton name
 *  HSVtoRBG
 * 
 * Funciton Parameters
 *  int H | Hue 0-360 deg
 *  int S | Saturation 0-255 (maps to 0-100%)
 *  int V | Value 0-255 (maps to 0-100%)
 * 
 * description
 *  generac HSV to RGB function I that uses intergers instead of float so 
 *  so will be some loss in percesion but it seems to work fairly well 
 *  how this works is hard to explain so I will refer to to the wikipedia
 *  article I used to create the funtion to begin with
 *  https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB
 * 
*/
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
/** 
 * Function Name 
 *  Demo1
 * 
 * Parameters 
 *  LEDarray *LED             | LED array to be modified
 *  uint8_t Previous Function | previous state of the main while (ex DEMO1 DEMO2 Reactive )
 * 
 * Description 
 *  State machine for the Demo1 Preset. If the previous function was not DEMO1 create a 
 *  Rainbow wave with the whole LED array as one perioid. If the previous state is DEMO 1 
 *  shift the wave once downward
*/

void Demo1(LEDArray *LED,uint8_t PreviousFunciton)
{
  
  int AngleDiff = 360/numLEDs; // delta HSV angle between each LED 
  uint32_t rgb =0;

    //if the array has not been initalized for demo1
   if(PreviousFunciton != DEMO1)
    {
      //for all LEDs
      for(int i = 0; i < LED->numLED;i++)
      {
     //get and set HSV to RGB value for each LED in the array    
      rgb = HSVtoRGB(i*AngleDiff,255,30);
      SetLED(LED,i, rgb & 0XFF, (rgb & 0xFF00)>>8, (rgb & 0xFF0000)>>16 );
      }

    }
    //shift LEDs
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