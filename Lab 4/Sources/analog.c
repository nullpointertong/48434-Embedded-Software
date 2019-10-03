/*!
** @file analog.c
** @brief Analog Signal Handling
**
**
**
** @author 12551242/12876417
** @date 2019-08-7
**
*/
/*!
**  @addtogroup analog_module analog module documentation
**  @{
*/
/* MODULE ANALOG */

#include "SPI.h"
#include "analog.h"
#include "Cpu.h"
#include "LEDs.h"

//used for spi setup
TAnalogInput Analog_Input[ANALOG_NB_INPUTS];

// used to store what data will be sent to each individual channel of the ADC
static uint16_t Input[ANALOG_NB_INPUTS];

// A flag used in the main to signal that a packet is ready to send
bool ADC_Update[ANALOG_NB_INPUTS];



bool Analog_Init(const uint32_t moduleClock)
{
  //configuration as specificed in the hints
  TSPIModule k70Startup;
  k70Startup.LSBFirst                  = false;
  k70Startup.baudRate                  = 1000000;
  k70Startup.changedOnLeadingClockEdge = false;
  k70Startup.continuousClock           = false;
  k70Startup.inactiveHighClock         = false;
  k70Startup.isMaster                  = true;


  // call SPI init
  SPI_Init(&k70Startup, moduleClock);
  

  //data to be sent to ADC, used in SPI_Exchange
  // for channel 0
  Input[0]  = 0x8400;
  // for channel 1
  Input[1]  = 0xC400;

  //sets the put pointer to the start of the array of the window
  Analog_Input[0].putPtr = Analog_Input[0].values;
  Analog_Input[1].putPtr = Analog_Input[1].values;

  return true;
}

bool Analog_Get(const uint8_t channelNb)
{
  //gets data from the ADC, runs twice to get the most recent data
  SPI_Exchange(Input[channelNb], Analog_Input[channelNb].putPtr);
  SPI_Exchange(Input[channelNb], Analog_Input[channelNb].putPtr);

  //moves pointer to the next spot in the array
  ++Analog_Input[channelNb].putPtr;
  // makes array circular
  if (Analog_Input[channelNb].putPtr == &Analog_Input[channelNb].values[ANALOG_WINDOW_SIZE])
    Analog_Input[channelNb].putPtr = Analog_Input[channelNb].values;

  //data is ready to be processed and sent to the PC
  ADC_Update[channelNb] = true;



   return true;
}



/*!
* @}
*/
