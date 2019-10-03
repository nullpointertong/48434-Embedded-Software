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
**  @addtogroup LED_module packet module documentation
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

  GPIOA_PCOR |= LED_ORANGE + LED_YELLOW + LED_GREEN + LED_BLUE;

  return true;
}


void LEDs_On(const TLED color)
{
  GPIOA_PDDR |= color;
}


void LEDs_Off(const TLED color)
{
  GPIOA_PSOR |= color;
}


void LEDs_Toggle(const TLED color)
{
  GPIOA_PTOR = color;
}
/*!
* @}
*/
