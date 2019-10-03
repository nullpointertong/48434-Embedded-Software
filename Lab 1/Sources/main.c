/* ###################################################################
**     Filename    : main.c
**     Project     : Lab1
**     Processor   : MK70FN1M0VMJ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 2.0
** @brief
**         Main module.
**         This module contains user's application code.
**
** @author 12551242/12876417
** @date 2019-08-7
**
*/
/*!
**  @addtogroup main_module main module documentation
**  @{
*/
/* MODULE main */

// CPU module - contains low level hardware initialization routines
#include "Cpu.h"
#include "Events.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "FIFO.h"
#include "packet.h"
#include "UART.h"

//Defining Baudrate as preprocessors
#define BAUDRATE 38400

//Defining Bit Masks as preprocessors
#define BIT_7_MASK 0x80
#define BIT_0_TO_6_MASK 0x7F
#define BIT_0_MASK 0x01

//Defining Packet Bytes as preprocessors

#define STARTUP_VALUES_COMMAND_BIT 0x04
#define TOWER_VERSION_COMMAND_BIT 0x09
#define TOWER_NUMBER_COMMAND_BIT 0x0B

static int16union_t Tower_Number;

void PacketRespond (const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3);

/*! @brief processes packet command and calls Packet_Put dependant on input packet command
 *
 *  @return bool - TRUE if a valid packet was processed success Flagfully.
 */
void PacketRespond (const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{

  //Used to determine if command is an ACK, if a packet is 'getting' or 'setting', if operation was a success
  bool ackFlag, getFlag, successFlag;

  ackFlag     = false;
  getFlag     = false;
  successFlag = false;


  //Used in determining what the command is in the switch statement
  uint8_t commandSwitch;

  //if bit 7 is up packet requires ACK
  if (command & BIT_7_MASK)
  {
    ackFlag=true;
     //ignores 7th bit in switch statement
    commandSwitch= command & BIT_0_TO_6_MASK;
  }
  else
  {
    commandSwitch=command;
    ackFlag=false;
  }

    //determines command in packet
  switch (commandSwitch)
  {
    //for get startup values
    case (STARTUP_VALUES_COMMAND_BIT):
    //determines if packet has expected variables
    if ((Packet_Parameter1==0)&&(Packet_Parameter2==0)&&(Packet_Parameter3==0))
    {
    //determines if packets sent successfully
      if (Packet_Put(STARTUP_VALUES_COMMAND_BIT, 0, 0, 0) && Packet_Put(TOWER_VERSION_COMMAND_BIT, 'v', 1, 0) &&
         Packet_Put(TOWER_NUMBER_COMMAND_BIT, 1, Tower_Number.s.Lo, Tower_Number.s.Hi))
	successFlag = true;
      else
        successFlag = false;
    }
      else
        successFlag = false;
    //sends ACK OR NACK needed
    if (ackFlag && successFlag)
      Packet_Put(command, parameter1, parameter2, parameter3);

    else if (ackFlag && !successFlag)
      Packet_Put(STARTUP_VALUES_COMMAND_BIT, parameter1, parameter2, parameter3);
    break;


    //For packet get version
    case (TOWER_VERSION_COMMAND_BIT):
    //determines if packet has expected variables
    if ((Packet_Parameter1==0x76)&&(Packet_Parameter2==0x78)&&(Packet_Parameter3==0xd))
    {
    //determines if packets sent successfully
      if (Packet_Put(TOWER_VERSION_COMMAND_BIT, 'v', 1, 0))
        successFlag = true;
      else
        successFlag = false;
    }
    else
      successFlag   = false;

    //Sends ACK or NACK if necessary
    if (ackFlag && successFlag)
      Packet_Put(command, parameter1, parameter2, parameter3);

    else if (ackFlag && !successFlag)
      Packet_Put(STARTUP_VALUES_COMMAND_BIT, parameter1, parameter2, parameter3);
    break;

    //for tower number get or set
    case (TOWER_NUMBER_COMMAND_BIT):
    // determines if packet is getting or setting
    if ((parameter1== 1) && (parameter2== 0) && (parameter3== 0))
      getFlag = true;

    else if (parameter1== 2)
      getFlag = false;
    // if 'setting' tower number is changed to that given in the packet
    if ((!getFlag)&&(parameter1== 2))
    {
       Tower_Number.s.Lo= parameter2;
       Tower_Number.s.Hi= parameter3;
       successFlag      =       true;
    }
    //if 'getting' sends packet to PC with reply
    else if (getFlag)
    {
      if (Packet_Put(TOWER_NUMBER_COMMAND_BIT, 1, Tower_Number.s.Lo, Tower_Number.s.Hi))
        successFlag  = true;
      else
        successFlag  = false;
    }
    else
      successFlag = false;

    //sends ACK or NACK if necessary
    if (ackFlag && successFlag)
      Packet_Put(command, parameter1, parameter2, parameter3);
    else if (ackFlag && !successFlag)
      Packet_Put(TOWER_NUMBER_COMMAND_BIT, parameter1, parameter2, parameter3);
    break;
  }

}

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/
  /* Write your code here */

  //if packet is initialized incorrectly return 1
  if (Packet_Init(BAUDRATE, CPU_BUS_CLK_HZ))
  {
    Tower_Number.s.Lo = 0xDA;
    Tower_Number.s.Hi = 0x4;
  //Start up values sent
    for (;;)
    {
      //Poll for UART
      UART_Poll();
      //If a valid byte from a packet is received respond according to
      //P Tower Serial Communication Protocol
      if (Packet_Get())
        PacketRespond (Packet_Command, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    }
  }
  else
  {
      return 1;
  }



/*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
