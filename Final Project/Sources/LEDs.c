/*!
** @file LEDs.c
** @brief This contains the functions for implementing the LEDs.
**
**
**
** @author 12551242/12876417
** @date 2019-08-7
**
*/
/*!
**  @addtogroup LED_module LED module documentation
**  @{
*/
/* MODULE LED */

#include "LEDs.h"
#include "Cpu.h"


bool LEDs_Init(void)
{
  //Enable Port A
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
  //Enable Multiplexing
  PORTA_PCR11 = PORT_PCR_MUX(1);
  PORTA_PCR28 = PORT_PCR_MUX(1);
  PORTA_PCR29 = PORT_PCR_MUX(1);
  PORTA_PCR10 = PORT_PCR_MUX(1);
  
  //Enable Drive Strength
  PORTA_PCR11 |= PORT_PCR_DSE_MASK;
  PORTA_PCR28 |= PORT_PCR_DSE_MASK;
  PORTA_PCR29 |= PORT_PCR_DSE_MASK;
  PORTA_PCR10 |= PORT_PCR_DSE_MASK;
  
  //Set the Register for all colours and then turn of the LEDs
  GPIOA_PDDR = LED_ORANGE + LED_YELLOW + LED_GREEN + LED_BLUE;
  GPIOA_PSOR = LED_ORANGE + LED_YELLOW + LED_GREEN + LED_BLUE;
  
  return true;
}


void LEDs_On(const TLED color)
{
  //Turn the LEDs on
  GPIOA_PCOR = color;
}


void LEDs_Off(const TLED color)
{
  //Turn the LEDs off
  GPIOA_PSOR = color;
}


void LEDs_Toggle(const TLED color)
{
  //Toggle between both
  GPIOA_PTOR = color;
}
/*!
* @}
*/
