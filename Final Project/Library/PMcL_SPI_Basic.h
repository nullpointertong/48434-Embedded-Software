/*! @file
 *
 *  @brief I/O routines for K70 SPI interface.
 *
 *  This contains the functions for operating the SPI (serial peripheral interface) module.
 *
 *  @author PMcL
 *  @date 2016-08-22
 */

#ifndef PMCL_SPI_BASIC_H
#define PMCL_SPI_BASIC_H

// new types
#include "types.h"

typedef struct
{
  bool isMaster;                   /*!< A Boolean value indicating whether the SPI is master or slave. */
  bool continuousClock;            /*!< A Boolean value indicating whether the clock is continuous. */
  bool inactiveHighClock;          /*!< A Boolean value indicating whether the clock is inactive low or inactive high. */
  bool changedOnLeadingClockEdge;  /*!< A Boolean value indicating whether the data is changed or captured on the leading clock edge. */
  bool LSBFirst;                   /*!< A Boolean value indicating whether the data is transferred LSB first or MSB first. */
  uint32_t baudRate;               /*!< The baud rate in bits/sec of the SPI clock. */
} TSPIModule;

/*! @brief Sets up the SPI before first use.
 *
 *  @param aSPIModule is a structure containing the operating conditions for the module.
 *  @param moduleClock The module clock in Hz.
 *  @return BOOL - true if the SPI module was successfully initialized.
 */
bool PMcL_SPI_Basic_Init(const TSPIModule* const aSPIModule, const uint32_t moduleClock);
 
/*! @brief Selects the current slave device
 *
 * @param slaveAddress The slave device address.
 */
void PMcL_SPI_Basic_SelectSlaveDevice(const uint8_t slaveAddress);

/*! @brief Simultaneously transmits and receives data.
 *
 *  @param dataTx is data to transmit.
 *  @param dataRx points to where the received data will be stored.
 */
void PMcL_SPI_Basic_Exchange(const uint16_t dataTx, uint16_t* const dataRx);

#endif
