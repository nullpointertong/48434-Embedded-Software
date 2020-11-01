/*!
** @file UART.c
** @brief I/O routines for UART communications on the TWR-K70F120M.
**
**  This contains the functions for operating the UART (serial port).
**
** @author 12551242/12876417
** @date 2019-09-16
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
#include "OS.h"

#define BIT_7_MASK 0x80
#define BIT_5_MASK 0X20
#define THREAD_STACK_SIZE 100

static TFIFO TxFIFO;
static TFIFO RxFIFO;

//Semaphores made for Receive and Transmit
OS_ECB * UARTTXSemaphore;
OS_ECB * UARTRXSemaphore;

//Init the Thread Stacks
static uint32_t UARTRxThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));
static uint32_t UARTTxThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));

//Extern the Functions
static void UARTTxThread (void *pData);
static void UARTRxThread (void *pData);

bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  OS_DisableInterrupts();

  //Create Semaphores for Transmit and Receive
  UARTTXSemaphore = OS_SemaphoreCreate(0);
  UARTRXSemaphore = OS_SemaphoreCreate(0);

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
  float decimalTuning = ((float) moduleClk / (float) (16 * baudRate));

  float fineTuning = (float) decimalTuning - (float) sbr.l;
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

  //Set up Interrupts
//  UART2_C2 |= UART_C2_TIE_MASK;
  UART2_C2 |= UART_C2_RIE_MASK;

  //irq=49,50 for Uart2
  NVICICPR1 = (1 << 17);
  // Enable interrupts from LPTMR module
  NVICISER1 = (1 << 17);

  NVICICPR1 = (1 << 18);
  // Enable interrupts from LPTMR module
  NVICISER1 = (1 << 18);

  FIFO_Init(&TxFIFO);
  FIFO_Init(&RxFIFO);


  //Create 2 threads to handle both Receive and Transmit
  OS_ERROR error;
  error = OS_ThreadCreate(UARTTxThread, NULL, & UARTTxThreadStack[THREAD_STACK_SIZE - 1], PRIORITY_TRANSMIT_THREAD);
  error = OS_ThreadCreate(UARTRxThread, NULL, & UARTRxThreadStack[THREAD_STACK_SIZE - 1], PRIORITY_RECEIVE_THREAD);

  OS_EnableInterrupts();
  return true;
}


bool UART_InChar(uint8_t *const dataPtr)
{
  OS_DisableInterrupts();
  //Enable Receive Interrupt
  //Check if receive FIFO is empty
  if (RxFIFO.NbBytes != 0)
  {
    //Take a byte from the FIFO if it isn't empty
    if (FIFO_Get(&RxFIFO, dataPtr))
    {
      OS_EnableInterrupts();
      return true;
    }
  }
  else
  {
    OS_EnableInterrupts();
    return false;
  }
}


bool UART_OutChar(const uint8_t data)
{
  OS_DisableInterrupts();
  //Enable Transmit Interrupt
  UART2_C2 |= UART_C2_TIE_MASK;
  //Checks if transmit FIFO is full
  if (TxFIFO.NbBytes != FIFO_SIZE)
  {
    //Add byte if FIFO isn't full
    if (FIFO_Put(&TxFIFO, data))
    {
      OS_EnableInterrupts();
      return true;
    }
  }
  else
  {
    OS_EnableInterrupts();
    return false;
  }
}


void UART_Poll(void)
{
  //check RDRF register
  if (UART2_S1 & BIT_5_MASK)
  {
    //if RDRF flag up, put data in RxFIFO
    uint8_t dataRegister = UART2_D;
    FIFO_Put(&RxFIFO, dataRegister);
  }

  //check TDRE register
  if (UART2_S1 & BIT_7_MASK)
  {
    //If TDRE flag up, retrieve data from TxFIFO
    FIFO_Get(&TxFIFO, (uint8_t * ) & UART2_D);
  }

}

void __attribute__ ((interrupt)) UART_ISR(void)
{
  //Service Interrupt
  OS_ISREnter();
  OS_ERROR error;

  //Call interrupt for UART2_RX_TX
  if (UART2_C2 & UART_C2_RIE_MASK)
  {
    if (UART2_S1 & UART_S1_RDRF_MASK)
    {
      error = OS_SemaphoreSignal(UARTRXSemaphore);
      UART2_C2 &= ~UART_C2_RIE_MASK;
      //TODO: RX char retrieves the information immediately
      //If error is found trigger interrupt
      if (error)
        PE_DEBUGHALT();
    }
  }

  // Transmit a character
  if (UART2_C2 & UART_C2_TIE_MASK)
  {
    if (UART2_S1 & UART_S1_TDRE_MASK)
    {
      //If error is found call debug
      error = OS_SemaphoreSignal(UARTTXSemaphore);
      UART2_C2 &= ~UART_C2_TIE_MASK;
      if (error)
        PE_DEBUGHALT();
    }
  }
  //Service Interrupt
  OS_ISRExit();
}

/* Thread used for transmitting data has optional Pdata pointer
 *
 */
static void UARTTxThread (void *pData)
{
  OS_ERROR error;
  for (;;)
  {
    error = OS_SemaphoreWait(UARTTXSemaphore,0);
    //If error detected interrupt OS
    if (error)
      PE_DEBUGHALT();
    // Clear TDRE flag by reading the status register
    if (UART2_S1 & UART_S1_TDRE_MASK)
    {
      //Reset Transmit Interrupt Bit

      //signal semaphore
    //If TDRE flag up, retrieve data from TxFIFO
      if (!(FIFO_Get(&TxFIFO, (uint8_t * ) & UART2_D)))
        UART2_C2 &= ~UART_C2_TIE_MASK;
      else
        UART2_C2 |= UART_C2_TIE_MASK;
    }
  }
}

/* Thread used for receiving data with optional pData pointer
 *
 */
static void UARTRxThread (void *pData)
{
  OS_ERROR error;
  for (;;)
  {
    error = OS_SemaphoreWait(UARTRXSemaphore,0);
    //If error detected interrupt OS
    if (error)
      PE_DEBUGHALT();
    //Clear RDRF flag by reading the status register
    if (UART2_S1 & UART_S1_RDRF_MASK)
    {
      //Reset Receive Interrupt Bit
      //if RDRF flag up, put data in RxFIFO
      uint8_t dataRegister = UART2_D;
      if (!(FIFO_Put(&RxFIFO, dataRegister)))
        UART2_C2 &= ~UART_C2_RIE_MASK;
      //OS_SemaphoreSignal(PacketSemaphore);
      else
        UART2_C2 |= UART_C2_RIE_MASK;
    }
  }
}

/*!
* @}
*/
