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
#include "LEDs.h"
#include "Flash.h"

//Defining Baudrate as preprocessors
#define BAUDRATE 115200

//Defining Bit Masks as preprocessors
#define BIT_7_MASK 0x80
#define BIT_0_TO_6_MASK 0x7F
#define BIT_0_MASK 0x01

//Defining Packet Bytes as preprocessors

#define STARTUP_VALUES_COMMAND_BIT 0x04
#define TOWER_VERSION_COMMAND_BIT 0x09
#define TOWER_NUMBER_COMMAND_BIT 0x0B
#define TOWER_PROGRAM_BYTE_COMMAND_BIT 0x07
#define TOWER_READ_BYTE_COMMAND_BIT 0x08
#define TOWER_MODE_COMMAND_BIT 0x0D


#define FLASH_BYTE_1 0x00080001LU
#define FLASH_BYTE_2 0x00080002LU
#define FLASH_BYTE_3 0x00080003LU
#define FLASH_BYTE_4 0x00080004LU
#define FLASH_BYTE_5 0x00080005LU
#define FLASH_BYTE_6 0x00080006LU


//Tower Number and Mode and made them pointers
static uint16union_t * TowerNumberPtr;
static uint16union_t * TowerModePtr;

void TowerInit();

void SendAck(bool ackFlag, bool successFlag, uint8_t command, uint8_t globalCommand, uint8_t parameter1, uint8_t parameter2,
        uint8_t parameter3);

bool CheckSuccess(uint8_t command, uint8_t parameter1, uint8_t parameter2, uint8_t parameter3);

bool StartupValuePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                   bool ackFlag);

bool GetVersionPacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                 bool ackFlag);

bool GetTowerNumberPacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2,
                          const uint8_t parameter3, bool ackFlag);

bool SetTowerNumberPacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2,
                          const uint8_t parameter3, bool ackFlag);

bool GetTowerModePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                   bool ackFlag);

bool SetTowerModePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                   bool ackFlag);

bool ProgramBytePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                  bool ackFlag);

bool ReadBytePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                    bool ackFlag);

void PacketRespond(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3);


void TowerInit()
{
  //Check what fields in the flash sector are full or empty
  freeByte = 0;
  uint8_t *checkByte = FLASH_DATA_START;
  uint8_t checkEmpty = 0;
  for (uint8_t count = 0; count < 8; ++count)
  {
    //Iterate through the code to check if spot is empty or not
    if (*checkByte == 0xFF)
    {
      ++checkEmpty;
    }
    else
      freeByte |= 1 << count;
    //If its not empty set the bit to 1.

    ++checkByte;
  }

  if (Flash_Default())
  {
    if (Flash_AllocateVar(&TowerNumberPtr, sizeof(*TowerNumberPtr)))
      Flash_Write16(TowerNumberPtr, 1242);
    if (Flash_AllocateVar(&TowerModePtr, sizeof(*TowerModePtr)))
      Flash_Write16(TowerModePtr, 1);
  }
  else
  {
    //Set the Tower Number and Mode if not empty
    TowerNumberPtr = (uint16_t *) FLASH_DATA_START;
    TowerModePtr = (uint16_t *) FLASH_BYTE_2;
  }

}

void SendAck(bool ackFlag, bool successFlag, uint8_t command, uint8_t globalCommand, uint8_t parameter1, uint8_t parameter2,
        uint8_t parameter3) 
{
  //Send an Ack for successful and Ackflag
  if (ackFlag && successFlag)
    Packet_Put(command, parameter1, parameter2, parameter3);
  else if (ackFlag && !successFlag)
    //Send an NACK flag if transfer unsuccessful
    Packet_Put(globalCommand, parameter1, parameter2, parameter3);
}

bool CheckSuccess(uint8_t command, uint8_t parameter1, uint8_t parameter2, uint8_t parameter3) 
{
  return Packet_Put(command, parameter1, parameter2, parameter3);
}

bool StartupValuePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                   bool ackFlag) 
{
  bool successFlag = false;
//determines if packet has expected variables
  if ((parameter1 == 0) && (parameter2 == 0) && (parameter3 == 0)) {
    //determines if packets sent successfully
    if (Packet_Put(STARTUP_VALUES_COMMAND_BIT, 0, 0, 0) &&
        Packet_Put(TOWER_VERSION_COMMAND_BIT, 'v', 1, 0) &&
        Packet_Put(TOWER_NUMBER_COMMAND_BIT, 1, TowerNumberPtr->s.Lo, TowerNumberPtr->s.Hi) &&
        Packet_Put(TOWER_MODE_COMMAND_BIT, 1, TowerModePtr->s.Lo, TowerModePtr->s.Hi))
      successFlag = true;
    else
      successFlag = false;
  } else
    successFlag = false;
  //sends ACK OR NACK needed
  SendAck(ackFlag, successFlag, command, STARTUP_VALUES_COMMAND_BIT, parameter1,
          parameter2, parameter3);
}


bool GetVersionPacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                 bool ackFlag) 
{
  bool successFlag = false;
  //determines if packet has expected variables
  if ((parameter1 == 0x76) && (parameter2 == 0x78) && (parameter3 == 0xd)) 
  {
    //determines if packets sent successfully
    successFlag = CheckSuccess(TOWER_VERSION_COMMAND_BIT, 'v', 1, 0);
  } 
  else
    successFlag = false;
  //Sends ACK or NACK if necessary
  SendAck(ackFlag, successFlag, command, TOWER_VERSION_COMMAND_BIT, parameter1, parameter2, parameter3);
}


bool GetTowerNumberPacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2,
                          const uint8_t parameter3, bool ackFlag)                           
{
  bool successFlag = false;
  successFlag = CheckSuccess(TOWER_NUMBER_COMMAND_BIT, 1, TowerNumberPtr->s.Lo, TowerNumberPtr->s.Hi);

  //sends ACK or NACK if necessary
  SendAck(ackFlag, successFlag, command, TOWER_NUMBER_COMMAND_BIT, parameter1, parameter2, parameter3);

}

bool SetTowerNumberPacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2,
                          const uint8_t parameter3, bool ackFlag) 
{
  bool successFlag = false;

  successFlag = Flash_Write16(TowerNumberPtr, Packet_Parameter23);

  //sends ACK or NACK if necessary
  SendAck(ackFlag, successFlag, command, TOWER_NUMBER_COMMAND_BIT, parameter1, parameter2, parameter3);

}

bool GetTowerModePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                   bool ackFlag)                    
{
  bool successFlag = false;
  successFlag = CheckSuccess(TOWER_MODE_COMMAND_BIT, 1, TowerModePtr->s.Lo, TowerModePtr->s.Hi);
  SendAck(ackFlag, successFlag, command, TOWER_NUMBER_COMMAND_BIT, parameter1, parameter2, parameter3);
}


bool SetTowerModePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                   bool ackFlag) 
{
  bool successFlag = false;

  successFlag = Flash_Write16(TowerModePtr, Packet_Parameter23);

  SendAck(ackFlag, successFlag, command, TOWER_NUMBER_COMMAND_BIT, parameter1, parameter2, parameter3);
}


bool ProgramBytePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                  bool ackFlag) 
{
  bool successFlag = false;
  // determines if packet is getting or setting
  if ((parameter1 >= 0) && (parameter1 <= 8) && (parameter2 == 0)) 
  {
    uint8_t *addressPtr;
    switch (parameter1) 
    {
      case (0):
        addressPtr = (uint8_t *) FLASH_DATA_START;
        break;

      case (1):
        addressPtr = (uint8_t *) FLASH_BYTE_1;
        break;

      case (2):
        addressPtr = (uint8_t *) FLASH_BYTE_2;
        break;

      case (3):
        addressPtr = (uint8_t *) FLASH_BYTE_3;
        break;

      case (4):
        addressPtr = (uint8_t *) FLASH_BYTE_4;
        break;

      case (5):
        addressPtr = (uint8_t *) FLASH_BYTE_5;
        break;

      case (6):
        addressPtr = (uint8_t *) FLASH_BYTE_6;
        break;

      case (7):
        addressPtr = (uint8_t *) FLASH_DATA_END;
        break;

      case (8):
        if (Flash_Erase())
        break;

      default:
        return false;
    }
    successFlag = Flash_Write8(addressPtr, parameter3);
  }

  //sends ACK or NACK if necessary
  SendAck(ackFlag, successFlag, command, TOWER_PROGRAM_BYTE_COMMAND_BIT, parameter1, parameter2, parameter3);
  return successFlag;
}


bool ReadBytePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,
                    bool ackFlag) 
{
  //TODO: use peter casting
  bool successFlag = false;

  if ((parameter1 >= 0) && (parameter1 <= 7) && (parameter2 == 0) && (parameter3 == 0)) {
    uint8_t addressLocation = 0;
    while (addressLocation != parameter1) {
      ++addressLocation;
    }

    uint8_t *data;
    switch (addressLocation) {
      case (0):
        data = (uint8_t *) FLASH_DATA_START;
        break;

      case (1):
        data = (uint8_t *) FLASH_BYTE_1;
        break;

      case (2):
        data = (uint8_t *) FLASH_BYTE_2;
        break;

      case (3):
        data = (uint8_t *) FLASH_BYTE_3;
        break;

      case (4):
        data = (uint8_t *) FLASH_BYTE_4;
        break;

      case (5):
        data = (uint8_t *) FLASH_BYTE_5;
        break;

      case (6):
        data = (uint8_t *) FLASH_BYTE_6;
        break;

      case (7):
        data = (uint8_t *) FLASH_DATA_END;
        break;
    }
    successFlag = CheckSuccess(TOWER_READ_BYTE_COMMAND_BIT, parameter1, 0, *data);
  } else
    successFlag = false;

  SendAck(ackFlag, successFlag, command, TOWER_READ_BYTE_COMMAND_BIT, parameter1, parameter2, parameter3);
}

void PacketRespond(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3) 
{
  //Used to determine if command is an ACK, if a packet is 'getting' or 'setting', if operation was a success
  bool ackFlag, getFlag, successFlag, validFlag;

  ackFlag = false;


  //Used in determining what the command is in the switch statement
  uint8_t commandSwitch;

  //if bit 7 is up packet requires ACK
  if (command & BIT_7_MASK) 
  {
    ackFlag = true;
    //ignores 7th bit in switch statement
    commandSwitch = command & BIT_0_TO_6_MASK;
  } 
  else 
  {
    commandSwitch = command;
    ackFlag = false;
  }
  //determines command in packet
  switch (commandSwitch) 
  {
    //for get startup values
    case (STARTUP_VALUES_COMMAND_BIT):
      StartupValuePacket(command, parameter1, parameter2, parameter3, ackFlag);
      break;

      //For packet get version
    case (TOWER_VERSION_COMMAND_BIT):
      GetVersionPacket(command, parameter1, parameter2, parameter3, ackFlag);
      break;

      //for tower number get or set
    case (TOWER_NUMBER_COMMAND_BIT):
      // determines if packet is getting or setting
      if ((parameter1 == 1) && (parameter2 == 0) && (parameter3 == 0))
        GetTowerNumberPacket(command, parameter1, parameter2, parameter3, ackFlag);
      else if (parameter1 == 2)
        SetTowerNumberPacket(command, parameter1, parameter2, parameter3, ackFlag);
      break;

      //For Programming a byte in the tower
    case (TOWER_PROGRAM_BYTE_COMMAND_BIT):
      ProgramBytePacket(command, parameter1, parameter2, parameter3, ackFlag);
      break;

      //For Reading a byte from the tower
    case (TOWER_READ_BYTE_COMMAND_BIT):
      getFlag = true;
      ReadBytePacket(command, parameter1, parameter2, parameter3, ackFlag);
      break;

    case (TOWER_MODE_COMMAND_BIT):
      // determines if packet is getting or setting
      if ((parameter1 == 1) && (parameter2 == 0) && (parameter3 == 0))
        GetTowerModePacket(command, parameter1, parameter2, parameter3, ackFlag);
      else if (parameter1 == 2)
        SetTowerModePacket(command, parameter1, parameter2, parameter3, ackFlag);
      // if 'setting' tower mode is changed to that given in the packet
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
  //Init Tower Number/Mode
  TowerInit();

  //if packet is initialized incorrectly return 1
  if (Packet_Init(BAUDRATE, CPU_BUS_CLK_HZ)) 
  {
    //Start up values sent
    for (;;) 
    {
      //Poll for UART
      UART_Poll();
      //If a valid byte from a packet is received respond according to
      //P Tower Serial Communication Protocol
      if (Packet_Get())
        PacketRespond(Packet_Command, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
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
