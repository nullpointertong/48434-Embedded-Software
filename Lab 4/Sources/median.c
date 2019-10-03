/*!
** @file median.c
** @brief median Signal Handling
**
**
**
** @author 12551242/12876417
** @date 2019-08-7
**
*/
/*!
**  @addtogroup median_module median module documentation
**  @{
*/
/* MODULE MEDIAN */

#include "median.h"
#include "types.h"

static void QuickSort(int16_t array[], uint32_t firstIndex, uint32_t lastIndex);
static void Combine(int16_t array[], uint16_t firstIndex, uint16_t middleIndex, uint16_t lastIndex);

/*! @brief Combines high and low array based on index
*
*  @return void
*/
static void Combine(int16_t array[], uint16_t firstIndex, uint16_t middleIndex, uint16_t lastIndex)
{
  uint32_t arrayIndex = firstIndex;
  uint32_t lowIndex   = 0;
  uint32_t highIndex  = 0;

  //Calculate Left and Right Array Sizes
  uint32_t highSize = lastIndex - middleIndex;
  uint32_t lowSize = middleIndex - firstIndex + 1;

  //Copy the arrays
  int16_t lowArray[lowSize];
  int16_t highArray[highSize];


  //Divide array into high and low
  for (uint8_t i = 0; i < highSize; ++i)
    highArray[i] = array[middleIndex + 1 + i];

  for (uint8_t i = 0; i < lowSize; ++i)
    lowArray[i] = array[firstIndex + i];

  //Merging the left and right Arrays
  while (lowIndex < lowSize & highIndex < highSize)
  {
    //put in smallest item into the array
    if (lowArray[lowIndex] <= highArray[highIndex])
        array[arrayIndex++] = lowArray[lowIndex++];
   else
     array[arrayIndex++] = highArray[highIndex++];
   }

  //Arrange the rest of the elements
  while (highIndex < highSize)
    array[arrayIndex++] = highArray[highIndex++];  while (highIndex < highSize)
      array[arrayIndex++] = highArray[highIndex++];
  while (lowIndex < lowSize)
    array[arrayIndex++] = lowArray[lowIndex++];


}

/*! @brief Splits Array into partitions
*
*  @return void
*/
static void QuickSort(int16_t array[], uint32_t firstIndex, uint32_t lastIndex)
{
  if (lastIndex > firstIndex)
  {
     //Take the middle of the provided Indexes
      int16_t partitionIndex = firstIndex + (lastIndex - firstIndex) / 2;

     //Sort first half and then the second half of the array based on index
     QuickSort(array, partitionIndex + 1, lastIndex);

     QuickSort(array, firstIndex, partitionIndex);
     //Recombine the result of both partitions
     Combine(array, firstIndex, partitionIndex, lastIndex);
  }

}

int16_t Median_Filter(const int16_t array[], const uint32_t size)
{
  //if size == 1, return array[0];
  //if size is 2, return avg of 2 values
  if(size == 1)
    return array[0];
  else if(size == 2)
    return (array[size/2] + array[size/2-1])/2;

  //Copy array made so Median can be found without modifying original array
  int16_t sortedArray[size];

  //Copy values into a copy array
  for (uint16_t iterator = 0 ; iterator < size ; iterator++)
    sortedArray[iterator] = array[iterator];

   //Sort the Array with the first and last index
   QuickSort(sortedArray, 0, size-1);

  //If Odd take middleIndex value(Remember C always rounds down)
  if (size % 2)
    return sortedArray[size/2];
  //If Even take the average
  else if (!size & 2)
    return (sortedArray[size/2] + sortedArray[size/2-1])/2;
}


/*!
* @}
*/
