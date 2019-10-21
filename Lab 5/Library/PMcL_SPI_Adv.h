/*! @file
 *
 *  @brief I/O routines for K70 SPI interface.
 *
 *  This contains the functions for operating the SPI (serial peripheral interface) module.
 *
 *  @author PMcL
 *  @date 2016-08-22
 */

#ifndef PMCL_SPI_ADV_H
#define PMCL_SPI_ADV_H

// new types
#include "types.h"

typedef enum
{
  SPI_CLOCK_POLARITY_INACTIVE_LOW = 0,
  SPI_CLOCK_POLARITY_INACTIVE_HIGH = 1
} TSPIClockPolarity;

typedef enum
{
  SPI_CLOCK_PHASE_CAPTURED_ON_LEADING = 0,
  SPI_CLOCK_PHASE_CHANGED_ON_LEADING = 1
} TSPIClockPhase;

typedef enum
{
  SPI_FIRST_BIT_MSB = 0,
  SPI_FIRST_BIT_LSB = 1
} TSPIFirstBit;

typedef struct
{
  uint8_t frameSize;                                           /*!< The frame size - valid range is 4 to 32. */
  TSPIClockPolarity clockPolarity;                             /*!< The clock polarity - either inactive low or inactive high. */
  TSPIClockPhase clockPhase;                                   /*!< The clock phase - either the data is changed or captured on the leading clock edge. */
  TSPIFirstBit firstBit;                                       /*!< The first bit - either the data is transferred LSB first or MSB first. */
  uint32_t delayAfterTransfer;                                 /*!< The delay after transfer, in nanoseconds. */
  uint32_t baudRate;                                           /*!< The baud rate in bits/sec of the SPI clock. */
} TCTAR;

typedef struct
{
  bool isMaster;                   /*!< A Boolean value indicating whether the SPI is master or slave. */
  bool continuousClock;            /*!< A Boolean value indicating whether the clock is continuous. */
  TCTAR ctar[2];                   /*!< An array of CTAR structures to initialise CTAR0 and CTAR1. */
} TSPIModule;

/*! @brief Sets up the SPI before first use.
 *
 *  @param aSPIModule is a structure containing the operating conditions for the module.
 *  @param moduleClk The module clock in Hz.
 *  @return BOOL - true if the SPI module was successfully initialized.
 */
bool PMcL_SPI_Adv_Init(const TSPIModule* const aSPIModule, const uint32_t moduleClock);
 
/*! @brief Selects the current slave device
 *
 * @param slaveAddress The slave device address.
 */
void PMcL_SPI_Adv_SelectSlaveDevice(const uint8_t slaveAddress);

/*! @brief Simultaneously transmits and receives data.
 *
 *  @param dataTx is data to transmit.
 *  @param dataRx points to where the received data will be stored.
 *  @param ctas selects the control and transfer attributes register to use for the exchange (0 or 1).
 *  @param continuousPCS selects whether the Peripheral Chip Select is continuous between transfers.
 */
void PMcL_SPI_Adv_Exchange(const uint16_t dataTx, uint16_t* const dataRx, uint8_t const ctas, bool const continuousPCS);

#endif
