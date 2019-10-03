/*!
** @file Flash.c
** @brief Code to create hardware abstraction layer for Flash.
**
**
**
** @author 12551242/12876417
** @date 2019-09-7
**
*/
/*!
**  @addtogroup Flash_module Flash module documentation
**  @{
*/
/* MODULE Flash */

#include "Flash.h"
#include "Cpu.h"
//#include "main.c"

#define FLASH_BYTE_1 0x00080001LU
#define FLASH_BYTE_2 0x00080002LU
#define FLASH_BYTE_3 0x00080003LU
#define FLASH_BYTE_4 0x00080004LU
#define FLASH_BYTE_5 0x00080005LU
#define FLASH_BYTE_6 0x00080006LU

#define ERASE_ 0X09


bool Flash_Init(void) 
{
  return true;
}


bool Flash_AllocateVar(volatile void **variable, const uint8_t size) 
{
  uint8_t byteMask;

  switch (size)
  {
    case (1):
    byteMask=1;
    break;

    case (2):
    byteMask=3;
    break;

    case (3):
    byteMask=15;
    break;
  }



    //If the size is 1 Allocate space for 1 Byte and if it is 2 allocate for 2 Bytes and if it's 
    //4 allocate space for 4 bytes using the freeByte variable.

      for (uint8_t count = 0; count < 8; count = count + size) 
      {
        if ((freeByte & (byteMask << count)) != (byteMask << count))
        {
          *variable = FLASH_DATA_START + count;
          freeByte |= byteMask << count;
          return true;
        }
      }


  return false;
}


bool Flash_Write32(volatile uint32_t *const address, const uint32_t data) 
{
  //Writes 32 bits of data by passing the 32 bits into Modify Phrase
  uint64union_t phrase;
  uint32_t *inFlash;

  if (!((uint32_t) address % 8))          // for address 0-- what about if address=1,2,3,5,6,7
  {
    inFlash = address + 1;
    phrase.s.Lo = data;
    phrase.s.Hi = *inFlash;
    //Call Modify Phrase when data is gathered together as 64 bit phrase.
    return ModifyPhrase(address, phrase);
  } 
  else if ((uint32_t) address % 8)   //for address 4
  {
    inFlash = address - 1;
    phrase.s.Hi = data;
    phrase.s.Lo = *inFlash;
    return ModifyPhrase(address - 1, phrase);
  }
  return false;
}


bool Flash_Write16(volatile uint16_t *const address, const uint16_t data)
{
  //Writes 16 bits of data by first arranging data into 16 bits and then calling Flash write 32
  uint32union_t word;
  uint16_t *inFlash;
  uint64union_t phrase;

  if (!((uint32_t) address % 4))   //for addresses: 0, 4---what if address=1,3,5,7?
  {
    inFlash = (address + 1);
    word.s.Lo = data;
    word.s.Hi = *inFlash;
    //Call Flasg 32 when 16 byte data is gathered together as 32 byte data
    return Flash_Write32(address, word.l);
  } 
  else if (((uint32_t) address % 4)) {   //for addresses: 2, 6
    inFlash = (address - 1);
    word.s.Hi = data;
    word.s.Lo = *inFlash;
    return Flash_Write32(address - 1, word.l);
  }
  else
    return false;
}


bool Flash_Write8(volatile uint8_t *const address, const uint8_t data) 
{
  //Writes 8 bits of data by first arranging the data and then calling Flash Write 16
  uint16union_t halfWord;
  uint8_t *inFlash;

  if (!((uint32_t) address % 2)) 
  {
    inFlash = address + 1;
    halfWord.s.Lo = data;
    halfWord.s.Hi = *inFlash;
    //Call Flash 16 when 8 Byte data is gathered together as 16 byte data
    return Flash_Write16(address, halfWord.l);
  } 
  else if ((uint32_t) address % 2)
  {
    inFlash = address - 1;
    halfWord.s.Hi = data;
    halfWord.s.Lo = *inFlash;
    return Flash_Write16(address - 1, halfWord.l);
  }
  else
    return false;
}


bool Flash_Erase(void) 
{
  uint32union4_t address;
  address.l = FLASH_DATA_START;

  //Set the command to 0x09 and then launch the command erasing the flash.
  TFCCOB commonObject;
  commonObject.command = 0x09;
  //Use Bitwise to set addresses to the first 24 bits of the address
  commonObject.F1 = address.l >> 16;
  commonObject.F2 = address.l >> 8;
  commonObject.F3 = address.l;
  commonObject.data0 = 0;
  commonObject.data1 = 0;
  commonObject.data2 = 0;
  commonObject.data3 = 0;
  commonObject.data4 = 0;
  commonObject.data5 = 0;
  commonObject.data6 = 0;
  commonObject.data7 = 0;
  return Launch(&commonObject);
}


static bool WritePhrase(const uint32_t address, const uint64union_t phrase) 
{
  TFCCOB commonObject;

  //Set the command to 0x07 and then launch the command writing into the sector. 
  commonObject.command = 0x07;

  //Use Bitwise to set address to the first 24 bits of the address.
  commonObject.F1 = address >> 16;
  commonObject.F2 = address >> 8;
  commonObject.F3 = address;


  //Use Bitwise to extract each byte from the Phrase
  commonObject.data0 = phrase.l >> 56;
  commonObject.data1 = phrase.l >> 48;
  commonObject.data2 = phrase.l >> 40;
  commonObject.data3 = phrase.l >> 32;

  commonObject.data4 = phrase.l >> 24;
  commonObject.data5 = phrase.l >> 16;
  commonObject.data6 = phrase.l >> 8;
  commonObject.data7 = phrase.l;


  //Launch the write command
  return Launch(&commonObject);
}


static bool ModifyPhrase(const uint32_t address, const uint64union_t phrase) 
{
  //Erase the sector and then write to it. 
  if (EraseSector(address))
    return WritePhrase(address, phrase);
}


static bool EraseSector(const uint32_t address) 
{
  //Check if address is empty it not erase the entire sector. 
  if (_FP(address) == 0xFFFFFFFFFFFFFFFF)
    return true;
  else
    return Flash_Erase();
}


static bool Launch(TFCCOB *commonObject)
{
  //put data into the FCCOOB registers

  //Set everything to Big Endian Format
  FTFE_FCCOB0 = commonObject->command;

  FTFE_FCCOB1 = commonObject->F1;
  FTFE_FCCOB2 = commonObject->F2;
  FTFE_FCCOB3 = commonObject->F3;

  FTFE_FCCOB8 = commonObject->data0;
  FTFE_FCCOB9 = commonObject->data1;
  FTFE_FCCOBA = commonObject->data2;
  FTFE_FCCOBB = commonObject->data3;

  FTFE_FCCOB4 = commonObject->data4;
  FTFE_FCCOB5 = commonObject->data5;
  FTFE_FCCOB6 = commonObject->data6;
  FTFE_FCCOB7 = commonObject->data7;

  //set CCIF flag to start command
  FTFE_FSTAT = FTFE_FSTAT_CCIF_MASK;
  //wait for CCIF bit to set and clear any error flags
  Flash_Wait();

  //check for errors;

  bool errorFlag = false;

  //Clear ACCERR Flag
  if (FTFE_FSTAT & FTFE_FSTAT_ACCERR_MASK) 
  {
    FTFE_FSTAT = FTFE_FSTAT_ACCERR_MASK;
    errorFlag = true;
  }
  //Clear FPVIOL Flag
  if (FTFE_FSTAT & FTFE_FSTAT_FPVIOL_MASK) 
  {
    FTFE_FSTAT = FTFE_FSTAT_FPVIOL_MASK;
    errorFlag = true;
  }


  if (!errorFlag)
    return true;
  else
    return false;
}


bool Flash_Default(void) 
{
  //Reset the byte tracking to 0
  freeByte = 0;
  uint8_t *checkByte = FLASH_DATA_START;
  uint8_t checkEmpty = 0;
  for (uint8_t count = 0; count < 8; ++count)
  {
     //Iterate through the code to check if spot is empty or not
     if (*checkByte == 0xFF)
     {
       ++checkEmpty;
     }
     else
       freeByte |= 1 << count;
     //If its not empty set the bit to 1.
     ++checkByte;
   }

  if (checkEmpty >= 8)
    return true;
  else
    return false;
}


void Flash_Wait(void) 
{
  //Check for the CCIF Flag and then if the Flag is raised clear all error flags
  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK)) 
  {
  }
}
/*!
* @}
*/
