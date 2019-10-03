/*!
** @file packet.c
** @brief Routines to implement packet encoding and decoding for the serial port.
**
**  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
**
** @author 12551242/
** @date 2019-08-7
**
*/
/*!
**  @addtogroup packet_module packet module documentation
**  @{
*/
/* MODULE packet */
#include "packet.h"
#include "UART.h"
#include "LEDs.h"

TPacket Packet;


bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
    //Initialization UART,LED and Packet Structure variable
    if(UART_Init(baudRate, moduleClk) && LEDs_Init())
    {
	//Turn on Orange LED for successful startup
	LEDs_On(LED_ORANGE);
	//Send Start up packet
	PacketRespond(0x04, 0, 0, 0);
	return true;
    }
    else
    {
      return false;
    }
}


bool Packet_Get(void)
{
  //checkSum for packet and bytes received are define as private global variables
  uint8_t checkSum;
  static uint8_t packetPositon= 0;
  //Create Finite State Machine using switch statement to take packet information byte by byte
  // While also using XOR to compute a checkSum
  switch (packetPositon)
  {
        case 0:
            if (UART_InChar(&Packet_Command))
                packetPositon++;
             else
                return false;
             
            break;
        
        case 1:
            if (UART_InChar(&Packet_Parameter1))
                packetPositon++;
             else
                return false;
            
            break;
        
        case 2:
            if (UART_InChar(&Packet_Parameter2))
                packetPositon++;
             else
                return false;
            break;
        
        case 3:
            if (UART_InChar(&Packet_Parameter3))
                packetPositon++;
             else
                return false;
            
            break;
        
        case 4:
            if (UART_InChar(&Packet_Checksum))
                packetPositon++;
             else
                return false;
            
            break;
            
//Check the packet checkSum and if packet sum is incorrect shift the parameters and feed in a new byte
        case 5:
            checkSum = Packet_Command ^ Packet_Parameter1 ^ Packet_Parameter2 ^ Packet_Parameter3;
            if ((checkSum == Packet_Checksum)&&(Packet_Checksum!= 0))
            {
                packetPositon = 0;
                return true;
            } 
            else if ((checkSum != Packet_Checksum)||(Packet_Checksum== 0))
              {
                Packet_Command = Packet_Parameter1;
                Packet_Parameter1 = Packet_Parameter2;
                Packet_Parameter2 = Packet_Parameter3;
                Packet_Parameter3 = Packet_Checksum;

                packetPositon--;
                return false;
              }
            break;
        default:
          packetPositon = 0;
          break;
    }
   return false;
}


bool Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
    //checkSum for packet is calculated using XOR
    uint8_t checkSum = command ^parameter1 ^parameter2 ^parameter3;
    //Return true if every byte of the packet can be transmitted to the UART correctly otherwise return false
    if (UART_OutChar(command) && UART_OutChar(parameter1) && UART_OutChar(parameter2) && UART_OutChar(parameter3) && UART_OutChar(checkSum))
        return true;
    else
        return false;
}

/*!
* @}
*/

