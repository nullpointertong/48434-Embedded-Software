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






//Declaring extern variables here
uint8_t Packet_Command;
uint8_t Packet_Parameter1;
uint8_t Packet_Parameter2;
uint8_t Packet_Parameter3;
uint8_t Packet_CheckSum;

/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz.
 *  @return bool - TRUE if the packet module was successfully initialized.
 */
bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
    //Initialization UART and Packet Structure variable
    UART_Init(baudRate, moduleClk);
    
    //Initialize tower number based on last 4 digits of student number '1242'
    

    //Send Start up packet
    PacketRespond(0x04, 0, 0, 0);
    
    return true;
}

/*! @brief Attempts to get a packet from the received data.
 *
 *  @return bool - TRUE if a valid packet was received.
 */
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
            if (UART_InChar(&Packet_CheckSum))
                packetPositon++;
             else
                return false;
            
            break;
            
//Check the packet checkSum and if packet sum is incorrect shift the parameters and feed in a new byte
        case 5:
            checkSum = Packet_Command ^ Packet_Parameter1 ^ Packet_Parameter2 ^ Packet_Parameter3;
            if ((checkSum == Packet_CheckSum)&&(Packet_CheckSum!= 0))
            {
                packetPositon = 0;
                return true;
            } else if ((checkSum != Packet_CheckSum)||(Packet_CheckSum== 0))
              {
                Packet_Command = Packet_Parameter1;
                Packet_Parameter1 = Packet_Parameter2;
                Packet_Parameter2 = Packet_Parameter3;
                Packet_Parameter3 = Packet_CheckSum;

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
//TODO:debug packet alignmnent

/*! @brief Builds a packet and places it in the transmit FIFO buffer.
 *
 *  @return bool - TRUE if a valid packet was sent.
 */
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



