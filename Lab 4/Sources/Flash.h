/*! @file
 *
 *  @brief Routines for erasing and writing to the Flash.
 *
 *  This contains the functions needed for accessing the internal Flash.
 *
 *  @author PMcL
 *  @date 2015-08-07
 */

#ifndef FLASH_H
#define FLASH_H

// new types
#include "types.h"

// FLASH data access
#define _FB(flashAddress)  *(uint8_t  volatile *)(flashAddress)
#define _FH(flashAddress)  *(uint16_t volatile *)(flashAddress)
#define _FW(flashAddress)  *(uint32_t volatile *)(flashAddress)
#define _FP(flashAddress)  *(uint64_t volatile *)(flashAddress)

// Address of the start of the Flash block we are using for data storage
#define FLASH_DATA_START 0x00080000LU
// Address of the end of the Flash block we are using for data storage
#define FLASH_DATA_END   0x00080007LU

typedef union
{
  uint32_t l;
  struct
  {
    uint8_t Lo;
    uint8_t LoMid;
    uint8_t HiMid;
    uint8_t Hi;
  } s;
} uint32union4_t;

//Command Object Structure.
typedef struct
{
  uint8_t command;
  uint8_t F1;
  uint8_t F2;
  uint8_t F3;
  //Bigend
  uint8_t data0;
  uint8_t data1;
  uint8_t data2;
  uint8_t data3;
  //Bigend
  uint8_t data4;
  uint8_t data5;
  uint8_t data6;
  uint8_t data7;
  //Bigend
} TFCCOB;

//This byte keeps track of every free byte within the Flash with each bit representing a byte
uint8_t freeByte;

/*! @brief Enables the Flash module.
 *
 *  @return bool - TRUE if the Flash was setup successfully.
 */
bool Flash_Init(void);
 
/*! @brief Allocates space for a non-volatile variable in the Flash memory.
 *
 *  @param variable is the address of a pointer to a variable that is to be allocated space in Flash memory.
 *         The pointer will be allocated to a relevant address:
 *         If the variable is a byte, then any address.
 *         If the variable is a half-word, then an even address.
 *         If the variable is a word, then an address divisible by 4.
 *         This allows the resulting variable to be used with the relevant Flash_Write function which assumes a certain memory address.
 *         e.g. a 16-bit variable will be on an even address
 *  @param size The size, in bytes, of the variable that is to be allocated space in the Flash memory. Valid values are 1, 2 and 4.
 *  @return bool - TRUE if the variable was allocated space in the Flash memory.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_AllocateVar(volatile void** variable, const uint8_t size);

/*! @brief Writes a 32-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write32(volatile uint32_t* const address, const uint32_t data);
 
/*! @brief Writes a 16-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 2-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write16(volatile uint16_t* const address, const uint16_t data);

/*! @brief Writes an 8-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write8(volatile uint8_t* const address, const uint8_t data);

/*! @brief Erases the entire Flash sector.
 *
 *  @return bool - TRUE if the Flash "data" sector was erased successfully.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Erase(void);

/*! @brief Writes data into the TFCCOB data type to be placed in registers
 *
 *  @return bool - TRUE if the Flash data is written without error
 *  @note Assumes Flash has been initialized.
 */
static bool WritePhrase(const uint32_t address, const uint64union_t phrase);

/*! @brief checks if the flash is erased before calling WritePhrase
 *
 *  @return bool - TRUE if the Flash data is written without error
 *  @note Assumes Flash has been initialized.
 */
static bool ModifyPhrase(const uint32_t address, const uint64union_t phrase);

/*! @brief Checks if flash is erased already before calling Flash_Erase
 *
 *  @return bool - TRUE if the Flash is erased without error
 *  @note Assumes Flash has been initialized.
 */
static bool EraseSector(const uint32_t address);

/*! @brief enters data into FCCOB registers based off Big Endian format
 *
 *  @return bool - TRUE if the Flash data is written without error
 *  @note Assumes Flash has been initialized.
 */
static bool Launch(TFCCOB *commonObject);

/*! @brief When data is erased, sets the Tower Number and Tower Mode to default values
 *
 *  @return bool - TRUE if the Flash data is written without error
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Default(void);

/*! @brief Waits until the CCIF flag goes up before continuing in the code
 *
 *  @note Assumes Flash has been initialized.
 */
void Flash_Wait(void);




#endif
