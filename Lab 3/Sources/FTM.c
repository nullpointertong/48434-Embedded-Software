/*!
** @file FTM.c
** @brief Routines to implement a FTM timer.
**
**  This contains the initialisation and application of an FTM timer
**
** @author 12551242/12876417
** @date 2019-09-16
/*!
**
**  @addtogroup FTM_module FTM module documentation
**  @{
*/
/* MODULE FTM */


#include "FTM.h"
#include "Cpu.h"

static TFTMChannel GFTMChannel[7];
//Array and just callback

bool FTM_Init()
{
  //Clock Gating Enabled for FTM0
  SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;
  
  //Enable FTM0
  FTM0_MODE |= FTM_MODE_FTMEN_MASK;
  
  //Write to CNTIN.
  FTM0_CNTIN = 0x0000;
  //Write to MOD.
  FTM0_MOD = 0xFFFF;
  //Write to CNT.
  FTM0_CNT = 0xFFFF;
  //Write to CLKS[1:0] (in SC)
  FTM0_SC |= FTM_SC_CLKS(2);
  
  FTM0_CONF |= FTM_CONF_BDMMODE(3);
  
  //NIVC irq = 62 for FTM0

  NVICICPR1 |= (1 << 30);
  // Enable interrupts from LPTMR module
  NVICISER1 |= (1 << 30);
  //Enable Interrupts from NVIC
  
  FTM0_MODE |= FTM_MODE_WPDIS_MASK;
  //Disable Write Protection
  
  return true;
}


bool FTM_Set(const TFTMChannel *const aFTMChannel)
{
  //Update Global Variable for Channel
  GFTMChannel[aFTMChannel->channelNb].callbackFunction = aFTMChannel->callbackFunction;
  GFTMChannel[aFTMChannel->channelNb].callbackArguments = aFTMChannel->callbackArguments;
  GFTMChannel[aFTMChannel->channelNb].channelNb = aFTMChannel->channelNb;
  
  //For Input/Output modes MSB/CPWMS is always 0 and Enable Channel Interrupts
  FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSB_MASK;
  FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_CHIE_MASK;
  FTM0_SC &= ~FTM_SC_CPWMS_MASK;
  
  //Set the Combine and Decapen to 0 so prepare Channel to become Input/Output
  if (aFTMChannel->channelNb == 7 || aFTMChannel->channelNb == 6)
  {
    FTM0_COMBINE &= ~FTM_COMBINE_DECAPEN3_MASK;
    FTM0_COMBINE &= ~FTM_COMBINE_COMBINE3_MASK;
  } else if (aFTMChannel->channelNb == 5 || aFTMChannel->channelNb == 4)
  {
    FTM0_COMBINE &= ~FTM_COMBINE_DECAPEN2_MASK;
    FTM0_COMBINE &= ~FTM_COMBINE_COMBINE2_MASK;
  } else if (aFTMChannel->channelNb == 3 || aFTMChannel->channelNb == 2)
  {
    FTM0_COMBINE &= ~FTM_COMBINE_DECAPEN1_MASK;
    FTM0_COMBINE &= ~FTM_COMBINE_COMBINE1_MASK;
  } else if (aFTMChannel->channelNb == 1 || aFTMChannel->channelNb == 0)
  {
    FTM0_COMBINE &= ~FTM_COMBINE_DECAPEN0_MASK;
    FTM0_COMBINE &= ~FTM_COMBINE_COMBINE0_MASK;
  }
  
  //Set the tower to an input mode depending on enum
  if (aFTMChannel->timerFunction == TIMER_FUNCTION_INPUT_CAPTURE)
  {
    //If input set ELSB/ESLA bits as [01] Capture on Rising Edge Only, [10] Capture on Falling Edge Only,
    //[11] Capture on Rising or Falling Edge
    FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSA_MASK;
    
    // for input DECAPEN = 0 COMBINE = 0  CPWMS = 0  MSnB:MSnA = 0:0, and ELSnB:ELSnA 0:0
    if (aFTMChannel->ioType.inputDetection == TIMER_INPUT_OFF)
    {
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK;
    }
    else if (aFTMChannel->ioType.inputDetection == TIMER_INPUT_RISING)
    {
      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK;
    }
    else if (aFTMChannel->ioType.inputDetection == TIMER_INPUT_FALLING)
    {
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK;
    }
    else if (aFTMChannel->ioType.inputDetection == TIMER_INPUT_ANY)
    {
      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
      FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK;
    }
    
  }
  else if (aFTMChannel->timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)
  {
    //Set the tower to an output mode depending on enum
    //Set MSA to 1
    FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_MSA_MASK;
    
    //Set action depending on enum
    FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
    
    if (aFTMChannel->ioType.outputAction == TIMER_OUTPUT_DISCONNECT)
    {
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
      FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK;
    }
    else if (aFTMChannel->ioType.outputAction == TIMER_OUTPUT_TOGGLE)
    {
      FTM0_POL ^= (1 << aFTMChannel->channelNb);
    }
    else if (aFTMChannel->ioType.outputAction == TIMER_OUTPUT_LOW)
    {
      FTM0_POL |= (1 << aFTMChannel->channelNb);
    }
    else if (aFTMChannel->ioType.outputAction == TIMER_OUTPUT_HIGH)
    {
      FTM0_POL &= ~(1 << aFTMChannel->channelNb);
    }
    
  }
  
}

bool FTM_StartTimer(const TFTMChannel *const aFTMChannel)
{
  //Update Global Variable for Channel
  GFTMChannel[aFTMChannel->channelNb].callbackFunction = aFTMChannel->callbackFunction;
  GFTMChannel[aFTMChannel->channelNb].callbackArguments = aFTMChannel->callbackArguments;
  GFTMChannel[aFTMChannel->channelNb].channelNb = aFTMChannel->channelNb;
  
  //Set delay if it is in output compare mode
  if (aFTMChannel->timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)
    FTM0_CnV(aFTMChannel->channelNb) = FTM0_CNT + aFTMChannel->delayCount;
  
  
  return true;
  
}


void __attribute__ ((interrupt)) FTM0_ISR(void)
{
  //Reset Channel Flag during interrupt while excuting a function with it's arguments

  for (uint8_t i = 0 ; i<8 ; i++)
  {
    if (FTM0_CnSC(i) & FTM_CnSC_CHIE_MASK)
    {
       FTM0_CnSC(i) &= ~FTM_CnSC_CHF_MASK;

      if (GFTMChannel[i].callbackFunction)
        (*GFTMChannel[i].callbackFunction)(GFTMChannel[i].callbackArguments);
    }
  }
}

/*!
* @}
*/

