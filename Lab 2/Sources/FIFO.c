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
  fifo->Start   = 0;
  fifo->End     = 0;
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
    return true;
  }
  else
    return false;
}

bool FIFO_Get(TFIFO *const fifo, uint8_t *const dataPtr)
{
  if (fifo->NbBytes != 0)
  {
     uint8_t * dataAddress = dataPtr;
     //Write to pointer address setting dataptr
    *(dataAddress) = fifo->Buffer[fifo->Start];
    fifo->NbBytes--;
     //Removes the number of bytes as a byte is taken out
    if (fifo->Start == FIFO_SIZE)
      fifo->Start = 0;
    else
      fifo->Start++;
     //Increments the start of the FIFO
    return true;
  }
  else
    return false;
}
/*!
* @}
*/

