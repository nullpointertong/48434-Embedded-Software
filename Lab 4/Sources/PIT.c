/*
 * PIT.c
 *
 *  Created on: 27 Aug 2019
 *      Author: 12876417
 */

/*!
**  @addtogroup PIT_module PIT module documentation
**  @{
*/
/* MODULE PIT */


#include "PIT.h"
#include "Cpu.h"

static void (*UserFunction)(void*);
static void* UserArguments;
static uint32_t ModuleClk;


bool PIT_Init(const uint32_t moduleClk, void (*userFunction)(void*), void* userArguments)
{
  //gate clocking
  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;
  //Update Private Global Variables
  UserFunction = userFunction;
  UserArguments = userArguments;
  ModuleClk = moduleClk;

  // turn on PIT and Freeze
  PIT_MCR &= ~ PIT_MCR_MDIS_MASK;
  PIT_MCR |= PIT_MCR_FRZ_MASK;



  //  PIT_LDVAL TSV-> start value for timer to count down in nanoseconds
  PIT_Set(10000, false);
  //  PIT_TCTRL TIE -> enables interrupts
  PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK;

//irq=68
   NVICICPR2 = (1 << 4);
   // Enable interrupts from LPTMR module
   NVICISER2 = (1 << 4);

  //  PIT_TCTRL TEN-> starts timer
  PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;

return true;

}


void PIT_Set(const uint32_t period, const bool restart)
{
  if(!restart)
    PIT_LDVAL0 = period*(ModuleClk/1000000)-1;
 else if(restart)
   {
     PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;

     PIT_LDVAL0 = period*(ModuleClk/1000000)-1;

     PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;
   }
    
}


void PIT_Enable(const bool enable)
{
//  MDIS turned on again
//  PIT_MCR MDIS -> enables module
  if (enable)
    PIT_MCR &= ~PIT_MCR_MDIS_MASK;
  else if (!enable)
    PIT_MCR |= PIT_MCR_MDIS_MASK;
}


void __attribute__ ((interrupt)) PIT_ISR(void)
{
  //  PIT_TFLG TIF -> timer interrupt flag (write 1 to clear)
  PIT_TFLG0 = PIT_TFLG_TIF_MASK;

  if (UserFunction)
    (*UserFunction)(UserArguments);
}

/*!
* @}
*/


