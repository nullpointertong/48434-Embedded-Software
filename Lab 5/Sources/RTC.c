/*
 * RTC.c
 *
 *  Created on: 27 Aug 2019
 *      Author: 12876417
 */

/*!
** @file RTC.c
** @brief Routines to implement the RTC module.
**
**  This contains the initialisation and application of the RTC module.
**
** @author 12551242/12876417
** @date 2019-09-16
*/
/*!
**  @addtogroup RTC_module RTC module documentation
**  @{
*/
/* MODULE RTC */

#include "RTC.h"
#include "OS.h"

static void (*UserFunction)(void *);

static void *UserArguments;

extern OS_ECB * RTCSemaphore;


bool RTC_Init(void (*userFunction)(void *), void *userArguments)
{
  UserFunction = userFunction;
  UserArguments = userArguments;
  //Update Private Global Variables

  //Create a Semaphore
  RTCSemaphore = OS_SemaphoreCreate(0);

  SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;
  
  // if TIF flag up, clear flag
   if (RTC_SR & RTC_SR_TIF_MASK )
     {
       RTC_SR &= ~RTC_SR_TCE_MASK;

       RTC_TSR = 0;

       RTC_SR |=RTC_SR_TCE_MASK;
     }

  // set software reset bit to test if locked
  RTC_CR |= RTC_CR_SWR_MASK;

 if (RTC_CR & RTC_CR_SWR_MASK)
    {
      RTC_CR &= ~RTC_CR_SWR_MASK;
      //Use 18pf load of capacitors (apparently in parallel)- RTC_CR
      RTC_CR |= RTC_CR_SC16P_MASK | RTC_CR_SC2P_MASK;
      //RTC_CR (OSCE) Oscillator enable
      RTC_CR |= RTC_CR_OSCE_MASK;
      //RTC_LR (crl)-> set to zero to lock crtl reg
      RTC_LR &= ~RTC_LR_CRL_MASK;
    
      //iterates for a 1000ms
      uint32_t iterator;
      //Delay for 1 second
      for (iterator = 0; iterator <= 0x600000; iterator++)
      {
      }
    
      //RTC_SR (TCE)  -> enables the 1sec rtc timer
      RTC_SR |= RTC_SR_TCE_MASK;
      //Lock down certain registers
      RTC_LR &= ~RTC_LR_CRL_MASK;
    
      //Bit must be zero in order to write to
      //Will be set when there is an error SR[TOF] or SR[TIF]
    
      // IRQ=67
      // NVIC number = IRQ/32, NVIC Bit = IRQ%32;
      //  Non-ipr: 2, ipr: 16
      // Clear any pending interrupts on LPTMR
    }
  
  //enables an interrupt every second
  RTC_IER |= RTC_IER_TSIE_MASK;
  NVICICPR2 = (1 << 3);
  // Enable interrupts from LPTMR module
  NVICISER2 = (1 << 3);
  
}


void RTC_Set(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
  //disable second counter
  RTC_SR &= ~RTC_SR_TCE_MASK;
  //set RTC time
  uint32_t secCombined = seconds + (minutes * 60) + (hours * 60 * 60);
  RTC_TSR = secCombined;
  //re-enable counter
  RTC_SR |= RTC_SR_TCE_MASK;
}


void RTC_Get(uint8_t *const hours, uint8_t *const minutes, uint8_t *const seconds)
{
  //transfer time from secs  to clock time
  uint32_t registerTime = RTC_TSR;
  uint32_t oldRegisterTime = RTC_TSR;

  //Time is not Synced correctly recursively call the function to try and re-read the time
  if (oldRegisterTime != registerTime)
  {
     RTC_Get(hours, minutes, seconds);
  }

  *hours = registerTime / 3600;
  registerTime = registerTime - (*hours) * (3600);
  
  *minutes = registerTime / 60;
  registerTime = registerTime - *minutes * 60;

  *seconds = registerTime;

}


void __attribute__ ((interrupt)) RTC_ISR(void)
{
  //Service Interrupt
  OS_ISREnter();

  //if clock= midnight, let the clock reset to 0
  if (RTC_TSR >= 0x15180)
  {
    RTC_Set(0, 0, 0);
  }
  

  OS_ERROR error;

  //Signal the RTC Semaphore to intiate callback
  error = OS_SemaphoreSignal(RTCSemaphore);
  //If error is found trigger interrupt
  if (error)
    PE_DEBUGHALT();


   //Service Interrupt
  OS_ISRExit();
}

/*!
* @}
*/

