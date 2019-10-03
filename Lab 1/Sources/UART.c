/*!
** @file UART.c
** @brief I/O routines for UART communications on the TWR-K70F120M.
**
**  This contains the functions for operating the UART (serial port).
**
** @author 12551242/
** @date 2019-08-7
**
*/
/*!
**  @addtogroup UART_module UART module documentation
**  @{
*/
/* MODULE packet */
#include "UART.h"
#include "Cpu.h"
#include "FIFO.h"

#define BIT_7_MASK 0x80
#define BIT_5_MASK 0X20

static TFIFO TxFIFO;
static TFIFO RxFIFO;

/*! @brief Sets up the UART interface before first use.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz.
 *  @return bool - TRUE if the UART was successfully initialized.
 */
bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  //Enable UART2/Port E
  //baudrate=38400, 8N1 frame
  //module clock 75mhz
  //SBR[12:0] + BRFD = 34
  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

  //sbrFineTuning is the sum of SBR + BRFD
  int16union_t sbr;
  sbr.l = moduleClk / (16 * baudRate);
  // Exact value of decimal place to fine tune

  uint8_t fineTuning = (moduleClk*2/(baudRate))%32;

  //Set Baud Rate Using the BDH and BDL
  UART2_BDH = UART_BDH_SBR(sbr.s.Hi);
  UART2_BDL = UART_BDL_SBR(sbr.s.Lo);

  //Set BRFD
  UART2_C4 |= UART_C4_BRFA(fineTuning);

  //Multiplexing Set up
  PORTE_PCR16 = PORT_PCR_MUX(3);
  PORTE_PCR17 = PORT_PCR_MUX(3);

  FIFO_Init(&TxFIFO);
  FIFO_Init(&RxFIFO);

  //Set Transmitter and Receiver up
  UART2_C2 |= UART_C2_TE_MASK;
  UART2_C2 |= UART_C2_RE_MASK;

  return true;
}

/*! @brief Get a character from the receive FIFO if it is not empty.
 *
 *  @param dataPtr A pointer to memory to store the retrieved byte.
 *  @return bool - TRUE if the receive FIFO returned a character.
 *  @note Assumes that UART_Init has been called.
 */
bool UART_InChar(uint8_t *const dataPtr)
{
    //Take a byte from the FIFO if it isn't empty
      return FIFO_Get(&RxFIFO, dataPtr);


}

/*! @brief Put a byte in the transmit FIFO if it is not full.
 *
 *  @param data The byte to be placed in the transmit FIFO.
 *  @return bool - TRUE if the data was placed in the transmit FIFO.
 *  @note Assumes that UART_Init has been called.
 */
bool UART_OutChar(const uint8_t data)
{

    //Add byte if FIFO isn't full
      return FIFO_Put(&TxFIFO, data);
}

/*! @brief Poll the UART status register to try and receive and/or transmit one character.
 *
 *  @return void
 *  @note Assumes that UART_Init has been called.
 */
void UART_Poll(void)
{
    //check RDRF register
  if (UART2_S1 & BIT_5_MASK)
  { //if RDRF flag up, put data in RxFIFO
    uint8_t dataRegister = UART2_D;
    FIFO_Put(&RxFIFO, dataRegister);
  }
    //check TDRE register
  if (UART2_S1 & BIT_7_MASK)
  {
    //If TDRE flag up, retrieve data from TxFIFO
    FIFO_Get(&TxFIFO, (uint8_t *)&UART2_D);
  }
}
