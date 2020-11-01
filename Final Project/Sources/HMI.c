/*
 * HMI.c
 *
 *  Created on: 24 Oct 2019
 *      Author: 12876417
 */

#include "HMI.h"

// Thread stacks
static uint32_t HMIThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));

//State of the HMI FSM
uint8_t State;

//Dormant Bool for the display
bool HMI_DormantDisplayBool;

////Dormant State Semaphore
//OS_ECB * DormantSemaphore;

//TODO: Change the check value into Q notation

bool HMI_Init()
{
  //Start In Dormant Display
  HMI_DormantDisplayBool = true;

//  //Dormant Semaphore created
//  DormantSemaphore = OS_SemaphoreCreate(0);

  //Enable Port D
  SIM_SCGC5  |= SIM_SCGC5_PORTD_MASK;

  //Multiplex to GPIO
  PORTD_PCR0  = PORT_PCR_MUX(1);

  //Enable Pull Resistor and interrupt
  PORTD_PCR0 |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IRQC(10);

  //Set to input so it reads SW1
  GPIOD_PDDR  &= ~1;

  //Enable Interrupts for the PORT IRQ = 90
  NVICICPR2 = (1 << 26);

  // Enable interrupts
  NVICISER2 = (1 << 26);

  //Clear Flag
  PORTD_PCR0 |= PORT_PCR_ISF_MASK;

  return true;
}

void HMI_RunTextDisplayThread()
{
  //Create a variable to use switch menu
  static uint8_t dormantTimer = 0;

  OS_DisableInterrupts();
  //Take in global state
  uint8_t localState = State;
  OS_EnableInterrupts();

  //Time captured in seconds
  uint16_t captureTimer = DEM_TimeSeconds;

  //Char buffer for sprintf
  char buffer[16] = {0};

  //Switch between displays reverting to dormant after 15 seconds
  uint8_t days,hours, minutes, seconds;
  switch(localState)
  {
    case (HMI_SELECT_DISPLAY_DORMANT):
    OS_DisableInterrupts();
    //Set the bool to true
    HMI_DormantDisplayBool = true;
    OS_EnableInterrupts();
//    sprintf(buffer, "");
//    HMI_Output(buffer,7);
    dormantTimer = 0;
    break;

    case (HMI_SELECT_DISPLAY_METER):
    //Set the bool to false
    HMI_DormantDisplayBool = false;

    //Calculates days to display
    //TODO: Do calculation in PIT?
    days = captureTimer/86400;
    captureTimer = captureTimer - (days) * (86400);
    hours = captureTimer / 3600;
    captureTimer = captureTimer - (hours) * (3600);
    minutes = captureTimer / 60;
    captureTimer = captureTimer - minutes * 60;
    seconds = captureTimer;

    //Day limit
    if (days <= 99)
      sprintf(buffer, "\n%d:%d:%d:%d",days,hours,minutes,seconds);
    else if (days > 99)
      sprintf(buffer, "\nxx:xx:xx:xx",days,hours,minutes,seconds);

    //Out Char
    HMI_Output(buffer,10);

    //While the button is not being pressed and 15 seconds haven't lapsed
    if (localState != HMI_SELECT_DISPLAY_METER)
    {
      dormantTimer = 0;
      HMI_RunTextDisplayThread();
    }

    if (localState == HMI_SELECT_DISPLAY_METER)
      dormantTimer++;

    if(dormantTimer >= 15)
    {
      dormantTimer = 0;
      OS_DisableInterrupts();
      //Se to dormant
      State = HMI_SELECT_DISPLAY_DORMANT;
      OS_EnableInterrupts();
    }

    break;

    case (HMI_SELECT_DISPLAY_AVERAGE_POWER):
    OS_DisableInterrupts();
    //Set the bool to false
    HMI_DormantDisplayBool = false;
    OS_EnableInterrupts();

    //Get Power
    sprintf(buffer,"\n %.3f", DEM_ConvertFrom32Q16(DEM_Power.l));

    HMI_Output(buffer,8);

    if (localState != HMI_SELECT_DISPLAY_AVERAGE_POWER)
    {
      dormantTimer = 0;
      HMI_RunTextDisplayThread();
    }

    if (localState == HMI_SELECT_DISPLAY_AVERAGE_POWER)
      dormantTimer++;

    if(dormantTimer >= 15)
    {
      dormantTimer = 0;
      OS_DisableInterrupts();
      //Set to dormant
      State = HMI_SELECT_DISPLAY_DORMANT;
      OS_EnableInterrupts();
    }
    break;

    case (HMI_SELECT_DISPLAY_TOTAL_ENERGY):
    OS_DisableInterrupts();
    //Set the bool to false
    HMI_DormantDisplayBool = false;
    OS_EnableInterrupts();

    if(DEM_ConvertFromQ(DEM_EnergyTotal.l,16) < 0.001)
      sprintf(buffer,"\n000.000");

//    DEM_ConvertFromQ(DEM_EnergyTotal.l,16)

    //Check that energy is within the 999kWh limit
    if (DEM_EnergyTotal.l < DEM_ConvertTo32Q16(999))
      sprintf(buffer,"\n%d.%3d", DEM_EnergyTotal.s.Hi, DEM_EnergyTotal.s.Lo);
    else
      sprintf(buffer,"\nxxx.xxx");

    //Output message
    HMI_Output(buffer,6);

    //Refresh if State is changed
    if (localState != HMI_SELECT_DISPLAY_TOTAL_ENERGY)
    {
      dormantTimer = 0;
      HMI_RunTextDisplayThread();
    }

    //Count down to Dormant
    if (localState == HMI_SELECT_DISPLAY_TOTAL_ENERGY)
      dormantTimer++;

    if(dormantTimer >= 15)
    {
      dormantTimer = 0;
      OS_DisableInterrupts();
      //Se to dormant
      State = HMI_SELECT_DISPLAY_DORMANT;
      OS_EnableInterrupts();
    }
    break;

    case (HMI_SELECT_DISPLAY_TOTAL_COST):
    OS_DisableInterrupts();
    //Set the bool to false
    HMI_DormantDisplayBool = false;
    OS_EnableInterrupts();

    //Check that costs are within the 99999.99 limit
    if (DEM_CostTotal < 1000000 && DEM_CostTotal > 0)
      sprintf(buffer,"\n%d.%.2f",DEM_CostTotal/100,DEM_CostTotal%100);
    else
      sprintf(buffer,"\nxxxx.xx");

    HMI_Output(buffer,7);

//    while (DEM_TimeSeconds - captureTimer <1 && localState == HMI_SELECT_DISPLAY_TOTAL_COST);
    if (localState != HMI_SELECT_DISPLAY_TOTAL_COST)
    {
      dormantTimer = 0;
      HMI_RunTextDisplayThread();
    }

    if (localState == HMI_SELECT_DISPLAY_TOTAL_COST)
      dormantTimer++;

    if(dormantTimer >= 15)
    {
      dormantTimer = 0;
      OS_DisableInterrupts();
      //Set to dormant
      State = HMI_SELECT_DISPLAY_DORMANT;
      OS_EnableInterrupts();
    }

    break;
  }
//  //End the thread
//  OS_ThreadDelete(PRIORITY_HMI_RUNTEXTDISPLAY_THREAD);
}

void HMI_Output(const char * message, const uint8_t index)
{
  //Iterate through the message and output
  for (uint8_t i = 0 ; i<index ; i++)
    (void) UART_OutChar(message[i]);
}

void __attribute__ ((interrupt)) HMI_ISR(void)
{
  //Service Interrupt
  OS_ISREnter();
  PORTD_PCR0 |= PORT_PCR_ISF_MASK;

  OS_ERROR error;

  OS_DisableInterrupts();
  if(State < HMI_SELECT_DISPLAY_TOTAL_COST)
  //Cycle to the next State
    State++;
  else if (State >= HMI_SELECT_DISPLAY_TOTAL_COST)
    State = HMI_SELECT_DISPLAY_METER;

  OS_EnableInterrupts();

  //Service Interrupt
  OS_ISRExit();
}



