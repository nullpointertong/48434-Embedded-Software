/*!
** @file FIFO.c
** @brief Routines to implement a FIFO buffer.
**
**  This contains the structure and "methods" for accessing a byte-wide FIFO
**
** @author 12551242/
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

/*! @brief Initialize the FIFO before first use.
 *
 *  @param fifo A pointer to the FIFO that needs initializing.
 *  @return bool - TRUE if the FIFO was successfully initialised
 */
bool FIFO_Init(TFIFO *const fifo)
{
  if (!fifo)
    return false;

  fifo->Start   = 0;
  fifo->End     = 0;
  fifo->NbBytes = 0;

  return true;
}

/*! @brief Put one character into the FIFO.
 *
 *  @param fifo A pointer to a FIFO struct where data is to be stored.
 *  @param data A byte of data to store in the FIFO buffer.
 *  @return bool - TRUE if data is successfully stored in the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Put(TFIFO *const fifo, const uint8_t data)
{
    //Checks to see if the FIFO is full
  if (fifo->NbBytes < FIFO_SIZE)
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

/*! @brief Get one character from the FIFO.
 *
 *  @param fifo A pointer to a FIFO struct with data to be retrieved.
 *  @param dataPtr A pointer to a memory location to place the retrieved byte.
 *  @return bool - TRUE if data is successfully retrieved from the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
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


