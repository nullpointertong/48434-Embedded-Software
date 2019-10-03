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
  //  float fineTuning = (moduleClk/(16 * baudRate)) ;
  float decimalTuning = ((float)moduleClk / (float)(16 * baudRate));

  float fineTuning = (float)decimalTuning - (float)sbr.l;
  fineTuning = (uint8_t)(fineTuning * 32);

  //Set Baud Rate Using the BDH and BDL
  UART2_BDH = UART_BDH_SBR(sbr.s.Hi);
  UART2_BDL = UART_BDL_SBR(sbr.s.Lo);

  //Set BRFD
  UART2_C4 |= UART_C4_BRFA(fineTuning);

  //Multiplexing Set up
  PORTE_PCR16 = PORT_PCR_MUX(3);
  PORTE_PCR17 = PORT_PCR_MUX(3);
  //PORTA_PCR18 = PORT_PCR_MUX(0);

  //Set Transmitter and Receiver up
  UART2_C2 |= UART_C2_TE_MASK;
  UART2_C2 |= UART_C2_RE_MASK;
  FIFO_Init(&TxFIFO);
  FIFO_Init(&RxFIFO);

  return true;
}


bool UART_InChar(uint8_t *const dataPtr)
{
    //Check if receive FIFO is empty
  if (RxFIFO.NbBytes != 0)
  {
    //Take a byte from the FIFO if it isn't empty
    if (FIFO_Get(&RxFIFO, dataPtr))
      return true;
  }
  else
    return false;
}


bool UART_OutChar(const uint8_t data)
{
    //Checks if transmit FIFO is full
  if (TxFIFO.NbBytes != FIFO_SIZE)
  {
    //Add byte if FIFO isn't full
    if (FIFO_Put(&TxFIFO, data))
      return true;
  }
  else
    return false;
}


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
/*!
* @}
*/
