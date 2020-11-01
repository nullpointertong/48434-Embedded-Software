/*!
** @file FIFO.c
** @brief Routines to implement a FIFO buffer.
**
**  This contains the structure and "methods" for accessing a byte-wide FIFO
**
** @author 12551242/12876417
** @date 2019-08-7
**
*/
/*!
**  @addtogroup FIFO_module FIFO module documentation
**  @{
*/
/* MODULE FIFO */
#include "FIFO.h"
#include "Cpu.h"


bool FIFO_Init(TFIFO *const fifo)
{
  //Create Semaphores in order to track status of FIFO
  fifo->EmptyStatusSempahore = OS_SemaphoreCreate(0);
  fifo->FullStatusSemaphore = OS_SemaphoreCreate(0);

  //Initialize the fifo indexes and the fifos
  fifo->Start = 0;
  fifo->End = 0;
  fifo->NbBytes = 0;
  if ((fifo->Start != NULL) && (fifo->End != NULL) && (fifo->NbBytes != NULL))
  {
    return true;
  }
  else
    return false;
}

bool FIFO_Put(TFIFO *const fifo, const uint8_t data)
{
  OS_ERROR error;
  OS_DisableInterrupts();
  //Checks to see if the FIFO is full
  if (fifo->NbBytes != FIFO_SIZE)
  {
    fifo->Buffer[fifo->End] = data;
    //FIFO folds over if pointer is at the end of buffer
    if (fifo->End == FIFO_SIZE)
      fifo->End = 0;
    else
      fifo->End++;
    //Increments the end of the FIFO/N of Bytes
    fifo->NbBytes++;
    OS_EnableInterrupts();
    //Signals Semaphore
    error = OS_SemaphoreSignal(fifo->FullStatusSemaphore);
    //If Error is found in Semaphore call interrupt
    if (error)
      PE_DEBUGHALT();
    return true;
  }
  else
  {
    OS_EnableInterrupts();
    //Waits until condition is valid
    error = OS_SemaphoreWait(fifo->FullStatusSemaphore,0);
    //If Error is found in Semaphore call interrupt for thread
    if (error)
      PE_DEBUGHALT();
  }
}

bool FIFO_Get(TFIFO *const fifo, uint8_t *const dataPtr)
{
  OS_ERROR error;
  OS_DisableInterrupts();
  if (fifo->NbBytes != 0)
  {
    uint8_t *dataAddress = dataPtr;
    //Write to pointer address setting dataptr
    *(dataAddress) = fifo->Buffer[fifo->Start];
    fifo->NbBytes--;
    //Removes the number of bytes as a byte is taken out
    if (fifo->Start == FIFO_SIZE)
      fifo->Start = 0;
    else
      fifo->Start++;
    //Increments the start of the FIFO
    //Signals Semaphore
    error = OS_SemaphoreSignal(fifo->EmptyStatusSempahore);
    //If Error is found in Semaphore call interrupt
    if (error)
      PE_DEBUGHALT();
    return true;
  }
  else
  {
    OS_EnableInterrupts();
    //Waits until conditions are valid
    error = OS_SemaphoreWait(fifo->EmptyStatusSempahore,0);
    //If error is found call interrupt
    if (error)
      PE_DEBUGHALT();
  }
}
/*!
* @}
*/

