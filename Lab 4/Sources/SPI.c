/*!
** @file SPI.c
** @brief SPI Signal Handling
**
**
**
** @author 12551242/12876417
** @date 2019-08-7
**
*/
/*!
**  @addtogroup SPI_module SPI module documentation
**  @{
*/
/* MODULE SPI */

#include "SPI.h"
#include "Cpu.h"


#define SEC_TO_MICRO_SEC 1000000
#define DELAY_IN_MICRO_SEC 5

static bool SPISetBaudRate(const uint32_t moduleClock, const uint32_t baudRate);
static bool SPISetDelay(const uint32_t moduleClock, const uint32_t delayUS );

bool SPI_Init(const TSPIModule* const aSPIModule, const uint32_t moduleClock)
{

  // Enable module clock for DSPI2  this is not SPI2
    SIM_SCGC3 |= SIM_SCGC3_DSPI2_MASK;
    // Enable clock gate port D
    SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK;
    // Enable clock gate to PORTE
    SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

    // Maps SPI2_PCS0 to PTD11
    PORTD_PCR11 = PORT_PCR_MUX(2);
    // Maps SPI2_SCK  to PTD12
    PORTD_PCR12 = PORT_PCR_MUX(2);
    // Maps SPI2_SOUT to PTD13
    PORTD_PCR13 = PORT_PCR_MUX(2);
    // Maps SPI2_SIN  to PTD14
    PORTD_PCR14 = PORT_PCR_MUX(2);
    // Maps SPI2_PCS1 to PTD15
    PORTD_PCR15 = PORT_PCR_MUX(2);

    // PTE5 is a GPIO
    PORTE_PCR5 = PORT_PCR_MUX(1);
    // PTE27 is a GPIO
    PORTE_PCR27 = PORT_PCR_MUX(1);
    // Initially clear output
    GPIOE_PCOR  = ((1 << 5) | (1 << 27));
    // Sets pins as outputs
    GPIOE_PDDR |= ((1 << 5) | (1 << 27));


    //set baud rate dynamically
    SPISetBaudRate(moduleClock, aSPIModule->baudRate);

    //set delay dynamically
    SPISetDelay(moduleClock, (uint32_t)DELAY_IN_MICRO_SEC );



    //Enable module clocks
    SPI2_MCR &= ~SPI_MCR_MDIS_MASK;
    // Doze mode disables the module
    SPI2_MCR &= ~SPI_MCR_DOZE_MASK;
    // PCS5 / ~PCSS used as peripheral chip select[5] signal
    SPI2_MCR &= ~SPI_MCR_PCSSE_MASK;
    // Receive FIFO overflow overwrite enable: incoming data ignored
    SPI2_MCR &= ~SPI_MCR_ROOE_MASK;

    // Halt serial transfers in debug mode
    SPI2_MCR |= SPI_MCR_FRZ_MASK;
    // Set Chip select for ADC to inactive high
    SPI2_MCR |= SPI_MCR_PCSIS(1);
    // Disable transmit FIFO
    SPI2_MCR |= SPI_MCR_DIS_TXF_MASK;
    // Disable receive FIFO
    SPI2_MCR |= SPI_MCR_DIS_RXF_MASK;

    // Set some CTAR0 fixed fields as per hints
    // Baud rate computed normally with 50/50 duty cycle
    SPI2_CTAR0 |= SPI_CTAR_DBR_MASK;
    // Pre-scaler value for assertion of PCS  and first edge SCK set to 1
    SPI2_CTAR0 |= SPI_CTAR_PCSSCK(0);
    // Pre-scaler 1 for delay between last edge SCK and negation of PCS.
    SPI2_CTAR0 |= SPI_CTAR_PASC(0);

    // Set some CTAR0 fixed fields as per hints
    // Baud rate computed normally with 50/50 duty cycle
    SPI2_CTAR0 |= SPI_CTAR_DBR_MASK;
    // Pre-scaler value for assertion of PCS  and first edge SCK set to 1
    SPI2_CTAR0 |= SPI_CTAR_PCSSCK(0);
    // Pre-scaler 2 for PCS to SCK delay (Master mode)
    SPI2_CTAR0 |= SPI_CTAR_CSSCK(0);

    // Set 16 bit frame size
    SPI2_CTAR0 |= SPI_CTAR_FMSZ(15);

    // isMaster
    if (aSPIModule->isMaster)
      // Enables master Mode
      SPI2_MCR |= SPI_MCR_MSTR_MASK;
    else
      // Enables slave Mode
      SPI2_MCR &= ~SPI_MCR_MSTR_MASK;

    // continuousClock
    if (aSPIModule->continuousClock)
      // Continuous SCK enabled
      SPI2_MCR |= SPI_MCR_CONT_SCKE_MASK;
    else
      // Continuous SCK disabled
      SPI2_MCR &= ~SPI_MCR_CONT_SCKE_MASK;

    // inactiveHighClock
    if (aSPIModule->inactiveHighClock)
      // Clock polarity inactive state value of SCK is high
      SPI2_CTAR0 |= SPI_CTAR_CPOL_MASK;
    else
      // Clock polarity inactive state value of SCK is low
      SPI2_CTAR0 &= ~SPI_CTAR_CPOL_MASK;


    // changedonLeadingClockEdge
    if (aSPIModule->changedOnLeadingClockEdge)
      // Data captured on leading edge of SCK and changed on following edge: Classic transfer format pg1848
      SPI2_CTAR0 |= SPI_CTAR_CPHA_MASK;
    else
      // Data changed on leading edge of SCK and captured on following edge
      SPI2_CTAR0 &= ~SPI_CTAR_CPHA_MASK;

    // LSBFirst
    if (aSPIModule->LSBFirst)
      // Data is transferred LSB first
      SPI2_CTAR0 |= SPI_CTAR_LSBFE_MASK;
    else
      // Data is transferred MSB first
      SPI2_CTAR0 &= ~SPI_CTAR_LSBFE_MASK;

    SPI_SelectSlaveDevice(7);

    return true;
}



void SPI_SelectSlaveDevice(const uint8_t slaveAddress)
{
  //adjust what PCS is set as dependent on the input
  switch (slaveAddress)
  {
    case 4:
      // no pins are set,
    break;

    case 5:
    GPIOE_PSOR = (1<<27);
    break;

    case 6:

    GPIOE_PSOR = (1<<5);
    break;

    
    case 7:
    GPIOE_PSOR = (1<<5)|(1<<27);
    break;


  }

}

void SPI_Exchange(const uint16_t dataTx, uint16_t* const dataRx)
{
  // While TFFF flag is 0
  while (!(SPI2_SR & SPI_SR_TFFF_MASK));

 // Push data and commands
 SPI2_PUSHR = SPI_PUSHR_PCS(1) | dataTx;

 // Write 1 to clear the TFFF flag
 SPI2_SR |= SPI_SR_TFFF_MASK;
 // Start transfer
 SPI2_MCR &= ~SPI_MCR_HALT_MASK;

 // While RDRF flag is 0
 while (!(SPI2_SR & SPI_SR_RFDF_MASK));

 // Stop transfer
 SPI2_MCR |= SPI_MCR_HALT_MASK;
 if (dataRx)
   *dataRx = (uint16_t)SPI2_POPR;

  // Write 1 to clear Receive FIFO drain flag
  SPI2_SR |= SPI_SR_RFDF_MASK;

}

/*! @brief calculates and sets the CTAR0 register with a baudrate closest to what was specifed in the input.
 *
 *  @param moduleClock The module clock rate in Hz.
 *  @param baudRate The desired baudrate rate in bit/s.
 *  @return bool - true if a baudRate was successfully allocated.
 */
static bool SPISetBaudRate(const uint32_t moduleClock, const uint32_t baudRate)
{
  //values for br and Pbr mapped via arrays
  uint32_t brValues[16] = {2, 4, 6, 8, 16, 32, 64, 128, 256, 512, 1024, 2048,
                           4096, 8192, 16384, 32768};
  uint32_t pbrValues[4] = {2, 3, 5, 7};
  
  //where the latest calculation will be stored
  uint32_t latestTest;
  
  //indexes for the current br and pbr value being used
  uint8_t brIndex = 0;
  uint8_t pbrIndex = 0;
  
  //places to put the configurations of BR and PBR that are closest to the baudRate so far
  uint32_t closestBaud = 0xFFFFFFFF;
  uint8_t closestBrIndex, closestPbrIndex;
  
  //start loop that will finish when all the values have been filtered through
  while (brIndex < 16)
  {
    //calculation to find the moduleClock for the current index of br and pbr
    latestTest = moduleClock / (brValues[brIndex] * pbrValues[pbrIndex]);
    
    // if the calculation equals the desired baudrate
    if (latestTest == baudRate)
    {
      //set the CTAR0 reg as the current indexes of br and pbr
      SPI2_CTAR0 |= SPI_CTAR_BR(brIndex) | SPI_CTAR_PBR(pbrIndex);
      //end function
      return true;
    }
    
    // if the calculation yields a value less than the desired baudRate
    if (latestTest < baudRate)
    {
      //if the deviation is smaller than the current closest calculation
      if ((baudRate - latestTest) < closestBaud)
      {
        //set the closest difference
        closestBaud = (baudRate - latestTest);
        //set the closest BR index
        closestBrIndex = brIndex;
        //set the closet PBR index
        closestPbrIndex = pbrIndex;
      }
      
    }
    // if the calculation yields a value less than the desired baudRate
    if (latestTest > baudRate)
    {
      //if the deviation is smaller than the current closest calculation
      if ((latestTest - baudRate) < closestBaud)
      {
        //set the closest difference
        closestBaud = (latestTest - baudRate);
        //set the closest BR index
        closestBrIndex = brIndex;
        //set the closet PBR index
        closestPbrIndex = pbrIndex;
      }
      
    }
    
    //increment PBRindex
    ++pbrIndex;
    //if pbr is beyond 7, increment br and reset pbr
    if (pbrIndex >= 4)
    {
      pbrIndex = 0;
      ++brIndex;
    }
  }
  
  // at the end of the loop, input closest baudRate
  SPI2_CTAR0 |= SPI_CTAR_BR(closestBrIndex) | SPI_CTAR_PBR(closestPbrIndex);
  
  return true;
}

// everything in micro SECONDS, not build for ns

/*! @brief calculates and sets the CTAR0 register with a after transfer delay
 * closest to what was specifed in the input that as a positive difference.
 *
 *  @param moduleClock The module clock rate in Hz.
 *  @param delayUS The desired delay  in microseconds.
 *  @return bool - true if a delay was successfully allocated.
 *  @note function cannot process a delay in a unit smaller that microseconds.
 */
static bool SPISetDelay(const uint32_t moduleClock, const uint32_t delayUS )
{
  // the values for DT and PDT bits, mapped via arrays
  uint32_t dtValues[16] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
  uint32_t pdtValues[4] = {1,3,5,7};

  // the value of the latest array
  uint32_t latestTest;
  
  //the value of the current index for the the value of DT and PDT
  uint8_t dtIndex = 0;
  uint8_t pdtIndex = 0;

  //data used to the store the current indexes closest to the desired delay
  uint32_t closestDelay = 0xFFFFFFFF;
  uint8_t closestDTIndex, closestPdtIndex;
  
  //loop will not end until all values have been completed
  while (dtIndex < 16)
  {
    // calculation for delay
    latestTest = ((dtValues[dtIndex] * pdtValues[pdtIndex] * SEC_TO_MICRO_SEC) /moduleClock);
    
    // if calculation yields a value equal to the desired delay, to the nearest whole number (net positive value)
    if (latestTest == delayUS)
    {
      // set CTAR0 reg based on current DT and PDT index values
      SPI2_CTAR0 |= SPI_CTAR_DT(dtIndex) | SPI_CTAR_PDT(pdtIndex);
      
      // end function
      return true;
    }
    
    // if calculation is greater than desired delay
    else if (latestTest > delayUS)
    {
      // if the calcultion is closer to the desired value than the previously recorded closest value
      if ((latestTest - delayUS) < closestDelay)
      {
        //closest to wanted value (with positive difference added)
        closestDelay = (latestTest - delayUS);
        
        //closest Dt index updated
        closestDTIndex = dtIndex;
        
        //closest pdt index updated
        closestPdtIndex = pdtIndex;
      }
      
    }
    
    //increment the index for PDT
    ++pdtIndex;
    // if the index reaches 4, reset and increment the dt index
    if (pdtIndex >= 4) {
      pdtIndex = 0;
      ++dtIndex;
    }
  }

  // at the end of the loop set the values of the CTAR0 reg
  SPI2_CTAR0 |= SPI_CTAR_DT(closestDTIndex) | SPI_CTAR_PDT(closestPdtIndex);

  return true;
}

/*!
* @}
*/
