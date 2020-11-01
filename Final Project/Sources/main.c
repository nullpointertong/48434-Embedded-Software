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
** @date 2019-09-16
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
#include "FTM.h"
#include "analog.h"
#include "PIT.h"
#include "analog.h"
#include "DEM.h"
#include "PMcL_Flash.h"
#include "HMI.h"

// Simple OS library
#include "OS.h"

//Defining Baudrate as preprocessor
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
#define TOWER_TIME_COMMAND_BIT 0x0C
#define TOWER_PROTOCOL_MODE_COMMAND_BIT 0x0A
#define TOWER_ANALOG_INPUT_COMMAND_BIT 0x50

//Flash Address
#define FLASH_BYTE_1 0x00080001LU
#define FLASH_BYTE_2 0x00080002LU
#define FLASH_BYTE_3 0x00080003LU
#define FLASH_BYTE_4 0x00080004LU
#define FLASH_BYTE_5 0x00080005LU
#define FLASH_BYTE_6 0x00080006LU

// Arbitrary thread stack size - big enough for stacking of interrupts and OS use.
#define THREAD_STACK_SIZE 100

//Checks if tower is in ASync or Sync mode
static volatile bool SyncBool;

/*//Checks if tower is in test mode
static volatile bool DEM_TestModeBool;*/

//Tower Number and Mode and made them pointers
static uint16union_t *TowerNumberPtr;
static uint16union_t *TowerModePtr;

// Thread stacks
static uint32_t TowerInitThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));
static uint32_t FTMCallBackThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));
static uint32_t GetTimePacketThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));
static uint32_t PitCallBackThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));
static uint32_t PacketThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));

//Binary Semaphores used as Signals
OS_ECB * PITSemaphore;
OS_ECB * PacketSemaphore;

//Frequency as private global
static uint16union_t Frequency;

void TowerInit();

bool FlashDefault(void);

void
SendAck(bool ackFlag, bool successFlag, uint8_t command, uint8_t globalCommand,
        uint8_t parameter1, uint8_t parameter2, uint8_t parameter3);

bool CheckSuccess(uint8_t command, uint8_t parameter1, uint8_t parameter2,
                  uint8_t parameter3);

bool StartupValuePacket(const uint8_t command, const uint8_t parameter1,
                        const uint8_t parameter2, const uint8_t parameter3,
                        bool ackFlag);

bool GetVersionPacket(const uint8_t command, const uint8_t parameter1,
                      const uint8_t parameter2, const uint8_t parameter3,
                      bool ackFlag);

bool GetTowerNumberPacket(const uint8_t command, const uint8_t parameter1,
                          const uint8_t parameter2, const uint8_t parameter3,
                          bool ackFlag);

bool GetTowerModePacket(const uint8_t command, const uint8_t parameter1,
                        const uint8_t parameter2, const uint8_t parameter3,
                        bool ackFlag);

bool ReadBytePacket(const uint8_t command, const uint8_t parameter1,
                    const uint8_t parameter2, const uint8_t parameter3,
                    bool ackFlag);

void PitCallBack (void* pData);

bool GetTimePacketDEM(const uint8_t command);

bool SelectTariff(const uint8_t parameter1);

bool TestMode(const uint8_t parameter1);

void PacketThread (void* pData);

void PacketRespond(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3);


/*! @brief Initializes Tower Number and Mode on startup
 *
 *  @return void
 */
void TowerInit()
{
//  //Intialize the Interupts
//  FTM_Init();
//  PIT_Init(CPU_BUS_CLK_HZ, PitCallBack, 0);
//  Analog_Init(CPU_BUS_CLK_HZ);
//  PMcL_Flash_Init();
//  //TODO: Implment correct threading
//  HMI_Init();

  //If All Modules don't Initialize Correctly halt the Cpu for a hardware error
  if (!FTM_Init() | !PIT_Init(CPU_BUS_CLK_HZ, PitCallBack, 0) | !Analog_Init(CPU_BUS_CLK_HZ) | !PMcL_Flash_Init() | !HMI_Init())
    PE_DEBUGHALT();

  DEM_PeriodSample[0]  = 1250;
  DEM_PeriodSample[1]  = 1250;

  //Set Tower to Async mode by default
  SyncBool = false;

  //Set Test mode of by default
  DEM_TestModeBool = false;

  //Check what fields in the flash sector are full or empty
  freeByte = 0;
  uint8_t *checkByte = FLASH_DATA_START;
  uint8_t checkEmpty = 0;
  for (uint8_t count = 0; count < 8; ++count)
  {
    //Iterate through the code to check if spot is empty or not
    if (*checkByte == 0xFF)
      ++checkEmpty;
    else
      freeByte |= 1 << count;
    //If its not empty set the bit to 1.
    ++checkByte;
  }
  
  if (FlashDefault())
  {
    //Convert tarrifs to fixed 8Q3 and is now 1 byte
    uint8_t tariff[5] = {(uint8_t) DEM_ConvertToQ(TOWER_TARIFF1_PEAK,3),(uint8_t) DEM_ConvertToQ(TOWER_TARIFF1_SHOULDER,3),
	(uint8_t) DEM_ConvertToQ(TOWER_TARIFF1_OFFPEAK,3),(uint8_t) DEM_ConvertToQ(TOWER_TARIFF2,3),(uint8_t) DEM_ConvertToQ(TOWER_TARIFF3,3)};

    //Create Pointers to set the values to
    volatile uint8_t * tariffPtr[5];

    //Write Tarriffs to flash
//    if (PMcL_Flash_AllocateVar((void *)&tariffPtr[0], sizeof(*tariffPtr[0])))
//      PMcL_Flash_Write8(tariffPtr[0], tariff[0]);
//    if (PMcL_Flash_AllocateVar((void *)&tariffPtr[1], sizeof(*tariffPtr[1])))
//      PMcL_Flash_Write8(tariffPtr[1], tariff[1]);
//    if (PMcL_Flash_AllocateVar((void *)&tariffPtr[2], sizeof(*tariffPtr[2])))
//      PMcL_Flash_Write8(tariffPtr[2], tariff[2]);
//    if (PMcL_Flash_AllocateVar((void *)&tariffPtr[3], sizeof(*tariffPtr[3])))
//      PMcL_Flash_Write8(tariffPtr[3], tariff[3]);
//    if (PMcL_Flash_AllocateVar((void *)&tariffPtr[4], sizeof(*tariffPtr[4])))
//      PMcL_Flash_Write8(tariffPtr[4], tariff[4]);
    if (PMcL_Flash_AllocateVar((void *)&tariffPtr[5], sizeof(*tariffPtr[5])))
         PMcL_Flash_Write8(tariffPtr[5], DEM_TariffSelect);
  }
  else
    //Set the Tariff if not empty
    DEM_TariffSelect = (uint8_t *) FLASH_BYTE_4;

  Packet_Init(BAUDRATE, CPU_BUS_CLK_HZ);

  //Create Threads from the init thread
  OS_ERROR error;
  error = OS_ThreadCreate(PitCallBack, NULL, &PitCallBackThreadStack[THREAD_STACK_SIZE - 1], PRIORITY_PIT_THREAD);
  error = OS_ThreadCreate(PacketThread, NULL, &PacketThreadStack[THREAD_STACK_SIZE - 1], PRIORITY_PACKETPOLL_THREAD);

  //End the init thread
  OS_ThreadDelete(0);
}

/*! @brief When data is erased, sets the Tower Number and Tower Mode to default values
 *
 *  @return bool - TRUE if the Flash data is written without error
 *  @note Assumes Flash has been initialized.
 */
bool FlashDefault(void)
{
  //Reset the byte tracking to 0
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

  if (checkEmpty >= 8)
    return true;
  else
    return false;
}


/*! @brief Initializes Send Ack if AckFlag and Packet was sent sucessfully
 *
 *  @return void
 */
void SendAck(bool ackFlag, bool successFlag, uint8_t command, uint8_t globalCommand,
        uint8_t parameter1, uint8_t parameter2, uint8_t parameter3)
{
  //Send an Ack for successful and Ackflag
  if (ackFlag && successFlag)
    Packet_Put(command, parameter1, parameter2, parameter3);
  else if (ackFlag && !successFlag)
    //Send an NACK flag if transfer unsuccessful
    Packet_Put(globalCommand, parameter1, parameter2, parameter3);
}

/*! @brief Checks if Packet is correctly transmitted
 *
 *  @return bool depending on if it was correctly transmitted or not
 */
bool CheckSuccess(uint8_t command, uint8_t parameter1, uint8_t parameter2,
                  uint8_t parameter3)
{
  return Packet_Put(command, parameter1, parameter2, parameter3);
}

/*! @brief processes and checks startup value packet command
 *
 *  @return void
 */
bool StartupValuePacket(const uint8_t command, const uint8_t parameter1,
                        const uint8_t parameter2, const uint8_t parameter3,
                        bool ackFlag)
{
  bool successFlag = false;
  //determines if packet has expected variables
  if ((parameter1 == 0) && (parameter2 == 0) && (parameter3 == 0))
  {
    //determines if packets sent successfully
    if (Packet_Put(STARTUP_VALUES_COMMAND_BIT, 0, 0, 0) && Packet_Put(TOWER_VERSION_COMMAND_BIT, 'v', 1, 0) &&
	Packet_Put(TOWER_NUMBER_COMMAND_BIT, 1, TowerNumberPtr->s.Lo,  TowerNumberPtr->s.Hi) &&
        Packet_Put(TOWER_MODE_COMMAND_BIT, 1, TowerModePtr->s.Lo, TowerModePtr->s.Hi) &&
	Packet_Put(TOWER_PROTOCOL_MODE_COMMAND_BIT, 1,SyncBool ,0 ))
      successFlag = true;
    else
      successFlag = false;
  }

    else
      successFlag = false;
      //sends ACK OR NACK needed
      SendAck(ackFlag, successFlag, command, STARTUP_VALUES_COMMAND_BIT, parameter1, parameter2, parameter3);
  
  return successFlag;
}

/*! @brief processes and checks the get version packet
*
*  @return void
*/
bool GetVersionPacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3,bool ackFlag)
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
  return successFlag;
}

/*! @brief processes the get Tower Number packet
*
*  @return void
*/
bool GetTowerNumberPacket(const uint8_t command, const uint8_t parameter1,const uint8_t parameter2, const uint8_t parameter3,bool ackFlag)
{
  bool successFlag = false;
  successFlag = CheckSuccess(TOWER_NUMBER_COMMAND_BIT, 1, TowerNumberPtr->s.Lo, TowerNumberPtr->s.Hi);
  
  //sends ACK or NACK if necessary
  SendAck(ackFlag, successFlag, command, TOWER_NUMBER_COMMAND_BIT, parameter1, parameter2, parameter3);

  return successFlag;
}

/*! @brief processes the get Tower Mode packet
*
*  @return void
*/
bool GetTowerModePacket(const uint8_t command, const uint8_t parameter1,const uint8_t parameter2, const uint8_t parameter3, bool ackFlag)
{
  bool successFlag = false;
  //gets data from flash
  successFlag = CheckSuccess(TOWER_MODE_COMMAND_BIT, 1, TowerModePtr->s.Lo, TowerModePtr->s.Hi);
  SendAck(ackFlag, successFlag, command, TOWER_NUMBER_COMMAND_BIT, parameter1, parameter2, parameter3);
  return successFlag;
}

/*! @brief processes the readBytePacket
*
*  @return void
*/
bool ReadBytePacket(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3, bool ackFlag)
{
  bool successFlag = false;
  //checks if parameters are valid
  if ((parameter1 >= 0) && (parameter1 <= 7) && (parameter2 == 0) && (parameter3 == 0))
  {
    uint8_t addressLocation = 0;
    while (addressLocation != parameter1)
      {
        ++addressLocation;
      }
    
    uint8_t *data;
    switch (addressLocation)
    {
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
  }
  else
    successFlag = false;
  
  SendAck(ackFlag, successFlag, command, TOWER_READ_BYTE_COMMAND_BIT, parameter1, parameter2, parameter3);
  return successFlag;
}

//TODO: Rewrite all header file briefs

/*! @brief to be called after 10ms pit interrupt, manages LED to go off every 500ms and gets data from SPI for ADC
 *
 */
void PitCallBack (void * pData)
{
  for(;;)
  {
    OS_SemaphoreWait(PITSemaphore,0);
    //PIT Timer to trigger 2nd callback
    static uint16 PitTimer = 0;
    
    //Sample Size
    static uint8_t sampleSize = 0;

    //Counter until the HMI gets set to dormant
    static uint8_t dormantTimer = 0;

    //Sample Size
    static uint8_t sampleCaptured = 0;

    //Take frequency and take phase angle
    static uint16_t phaseAngle;

    //Is a new cycle starting?
    bool cycleStarting = false; 

    //Increment when sample Size is bellow 16
    if (sampleSize < 16)
      sampleSize++;

    OS_DisableInterrupts();
    //Update Frequency Calculation
    DEM_CalculateFrequencyAndPhase(&cycleStarting,&Frequency.l,&phaseAngle, DEM_IndexHead, &sampleCaptured);
    OS_EnableInterrupts();

    if(cycleStarting)
    {
      OS_DisableInterrupts();
      //TODO: Sum Energy and divide energy into total and period
      //Energy during Cycle Period
      int32_t EnergyInPeriod =  DEM_CalculateEnergy(DEM_VoltageSin,DEM_CurrentSin, sampleSize);
      int32_t CostInPeriod   = DEM_CalculateCost(EnergyInPeriod);

      DEM_EnergyTotal.l += EnergyInPeriod;
      DEM_CostTotal    += CostInPeriod;

      cycleStarting = false;
    }

    //Update RMS Voltage,Current and powerfactor here
    //TODO: Check Position of vars
    DEM_RMSVoltage.l  = DEM_CalculateRMS(DEM_VoltageSin,sampleSize);
    DEM_RMSCurrent.l  = DEM_CalculateRMS(DEM_CurrentSin,sampleSize);

    //Convert to 32Q16
    DEM_PowerFactor.l = (int32_t)DEM_ConvertToQ(DEM_CustomCos[phaseAngle],16);


    DEM_CalculatePower();


    //Add 1 Cycle Worth of Time
    DEM_Timer++;

//    TimeSeconds += PeriodSample[1];
    DEM_TimeArray[0] = DEM_TimeArray[1];
    DEM_TimeArray[1] = DEM_TimeSeconds;

    //Count down till 1s have passed than trigger led, fix the time to sample period
    if (PitTimer < 800)
      PitTimer++;
    else
    {
      HMI_RunTextDisplayThread();
      //If we are not in test mode tick at once per second
      if (!DEM_TestModeBool)
	DEM_TimeSeconds++;
      //If Test Mode is active every second is an hour and generate a waveform based of input
      else if (DEM_TestModeBool)
	DEM_TimeSeconds += 3600;

      // dormantTimer++;

      // //Update Values every second
      // if (HMI_DormantDisplayBool)
      //   HMI_RunTextDisplayThread;

      PitTimer = 0;
      LEDs_Toggle(LED_GREEN);
      OS_EnableInterrupts();
    }
  }
}

/*! @brief Sends days, hours, minutes and seconds into packets
*
*  @return void
*/
bool GetTimePacketDEM(const uint8_t command)
{
  uint8_t days,hours, minutes, seconds;

  //TODO: Make the calculation in the callback?
  uint64_t localTimeinSeconds = DEM_TimeSeconds;

  days = localTimeinSeconds/86400;
  localTimeinSeconds = localTimeinSeconds - (days) * (86400);

  hours = localTimeinSeconds / 3600;
  localTimeinSeconds = localTimeinSeconds - (hours) * (3600);

  minutes = localTimeinSeconds / 60;
  localTimeinSeconds = localTimeinSeconds - minutes * 60;

  seconds = localTimeinSeconds;

  if(command == TOWER_TIME1_COMMAND_BIT)
  {
    Packet_Put(TOWER_TIME1_COMMAND_BIT, seconds, minutes, 0);
    return true;
  }
  else if(command == TOWER_TIME2_COMMAND_BIT)
  {
    Packet_Put(TOWER_TIME2_COMMAND_BIT, hours, days, 0);
    return true;
  }

  return false;
}

/*! @brief Selects tariff
 *
 *  @return void
 */
bool SelectTariff (const uint8_t parameter1)
{
  if(parameter1 > 0 && parameter1 < 4)
  {
    DEM_TariffSelect = parameter1;
    //Wipe Tariff and keep track of the new tariff in flash
   (void) PMcL_Flash_Erase();

   volatile uint8_t * tariffPtr;

   if (PMcL_Flash_AllocateVar((void *)&tariffPtr, sizeof(*tariffPtr)))
     PMcL_Flash_Write8(tariffPtr, DEM_TariffSelect);

    return true;
  }

}

/*! @brief This function Selects Test Mode acclerating the timer and using analog put to generate a test wave
 *
 *  @return void
 */
bool TestMode(const uint8_t parameter1)
{
  //Set the global bool to true
  OS_DisableInterrupts();
  DEM_TestModeBool = parameter1;
  OS_EnableInterrupts();

  return true;
}

/*! @brief This function polls for Packet data
 *
 *  @return void
 */
void PacketThread (void* pData)
{
  //Start up values sent
  for (;;)
  {
    if (Packet_Get(pData))
    {
      // OS_EnableInterrupts();
      PacketRespond(Packet_Command, Packet_Parameter1, Packet_Parameter2,Packet_Parameter3);
    }
  }
}

/*! @brief processes packet command and calls Packet_Put dependant on input packet command
 *
 *  @return void
 */
void PacketRespond(uint8_t command, uint8_t parameter1, uint8_t parameter2, uint8_t parameter3)
{
  //Used to determine if command is an ACK, if a packet is 'getting' or 'setting', if operation was a success and if the packet is the first startup
  bool ackFlag, getFlag, successFlag;
  
  ackFlag = false;
  successFlag = false;
  
  //Used in determining what the command is in the switch statement
  uint8_t commandSwitch;
  
  //TODO: Get and Set?

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

  //Energy in Wh
  uint64union_t displayKw;

  //Frequency in Hz * 10
  uint64union_t displayHz;

  //PowerFactor in  * 1000
  uint64union_t displayPowerF;

  //determines command in packet
  switch (commandSwitch)
  {
    //Note Parameter 3 is the get/set bit where 0 is set and 1 is get
    case (TOWER_TEST_MODE_COMMAND_BIT):
    // process asynch or synch mode
    if (!parameter3)
      successFlag = TestMode(parameter1);
    else if (parameter3)
      Packet_Put(TOWER_TEST_MODE_COMMAND_BIT, parameter1,0,0);

    if (successFlag)
      Packet_Put(TOWER_TEST_MODE_COMMAND_BIT, parameter1,0,0);
    break;

    case (TOWER_TARIFF_COMMAND_BIT):
    // process asynch or synch mode
    if (!parameter3)
      successFlag = SelectTariff(parameter1);
    else if (parameter3)
      Packet_Put(TOWER_TARIFF_COMMAND_BIT, parameter1,0,0);
    if (successFlag)
      Packet_Put(TOWER_TARIFF_COMMAND_BIT, parameter1,0,0);
    break;

    case (TOWER_TIME1_COMMAND_BIT):
    // process asynch or synch mode
    successFlag = GetTimePacketDEM(TOWER_TIME1_COMMAND_BIT);
    break;

    case (TOWER_TIME2_COMMAND_BIT):
    // process asynch or synch mode
    successFlag = GetTimePacketDEM(TOWER_TIME2_COMMAND_BIT);
    break;

    case (TOWER_POWER_COMMAND_BIT):
    // process power packet
    Packet_Put(TOWER_POWER_COMMAND_BIT,DEM_Power.s.Lo,DEM_Power.s.Hi, 0);
    break;

    case (TOWER_ENERGY_COMMAND_BIT):
    // Send packet out
    displayKw.l = 1000 * DEM_EnergyTotal.l;
    Packet_Put(TOWER_POWER_COMMAND_BIT,displayKw.s.Lo,displayKw.s.Hi, 0);
    break;

    case (TOWER_COST_COMMAND_BIT):
    Packet_Put(TOWER_POWER_COMMAND_BIT,DEM_CostTotal%100 ,DEM_CostTotal/100, 0);
    break;

    case (TOWER_FREQUENCY_COMMAND_BIT):
    //As the frequency is measured in millihertz and we want in * 10
    displayHz.l = Frequency.l/100;
    Packet_Put(TOWER_FREQUENCY_COMMAND_BIT, displayHz.s.Lo, displayHz.s.Hi, 0);
    break;

    case (TOWER_VOLTAGE_COMMAND_BIT):
    Packet_Put(TOWER_FREQUENCY_COMMAND_BIT, DEM_RMSVoltage.s.Lo, DEM_RMSVoltage.s.Hi, 0);
    break;

    case (TOWER_CURRENT_COMMAND_BIT):
    Packet_Put(TOWER_FREQUENCY_COMMAND_BIT, DEM_RMSCurrent.s.Lo, DEM_RMSCurrent.s.Hi, 0);
    break;

    case (TOWER_POWERFACTOR_COMMAND_BIT):
    //Per Spec multiple by a thousand
    displayPowerF.l =  DEM_PowerFactor.l * 1000;
    Packet_Put(TOWER_FREQUENCY_COMMAND_BIT, displayPowerF.s.Lo, displayPowerF.s.Hi, 0);
    break;
   //TODO: Make Packetput more consistent
  }
  
  //Turn Blue LED on for 1 Second
  if (successFlag)
  {
     LEDs_On(LED_BLUE);
     TFTMChannel FTMChannel;
     FTMChannel.callbackFunction = LEDs_Off;
     FTMChannel.callbackArguments = LED_BLUE;
     FTMChannel.delayCount = CPU_MCGFF_CLK_HZ_CONFIG_0;
     FTMChannel.channelNb = 0;
     FTMChannel.timerFunction = TIMER_FUNCTION_OUTPUT_COMPARE;
     TFTMChannel *FTMChannel_PTR = &FTMChannel;
     //If Packet Received Successfully Setup a timer using channel 0 and output compare

     //Set the Timer
     FTM_Set(FTMChannel_PTR);
     //Start the delay for the interrupt
     FTM_StartTimer(FTMChannel_PTR);
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

  //Init Floating Point Functionality for Sprintf
  char buffer[16] = {0};
  sprintf(buffer, "\n%f",0.00000);

  OS_ERROR error;
  //Init Tower Number/Mode
  bool startUpFlag = false;

  //Init the Operating System
  OS_Init(CPU_BUS_CLK_HZ,false);
  
  //Create an Init thread to init the other threads
  error = OS_ThreadCreate(TowerInit, NULL, &TowerInitThreadStack[THREAD_STACK_SIZE - 1], 0);

  //Start the Operating System
  OS_Start();

/*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
  PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for (;;) {}
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
