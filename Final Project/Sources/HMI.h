/*
 * HMI.h
 *
 *  Created on: 24 Oct 2019
 *      Author: 12876417
 */

#ifndef HMI_H_
#define HMI_H_

// new types
#include "types.h"
#include "OS.h"
#include "Cpu.h"
#include "DEM.h"
#include "UART.h"
#include <stdio.h>

//Define the THread size 
#define THREAD_STACK_SIZE 100

//Make the states macros for readability
#define HMI_SELECT_DISPLAY_DORMANT 0
#define HMI_SELECT_DISPLAY_METER 1
#define HMI_SELECT_DISPLAY_AVERAGE_POWER 2
#define HMI_SELECT_DISPLAY_TOTAL_ENERGY 3
#define HMI_SELECT_DISPLAY_TOTAL_COST 4

//Checks if the HMI is in Dormant state
extern bool HMI_DormantDisplayBool;

//State of the HMI FSM
extern uint8_t State;

/*! @brief Sets up the HMI and SW1 before first use.
 *
 *  @return TRUE if successful
 */
bool HMI_Init();

/*! @brief Generates the display text and values
 *
 *  @return void
 */
void HMI_RunTextDisplayThread();

/*! @brief Outputs the text display using Uart Out Char
 *  @param message message to be outchared by Uart
 *  index number of chars in string
 *  @return void
 */
void HMI_Output(const char * message, const uint8_t index);


/*! @brief Interrupt service routine for the UART.
 *
 *  @note assumes button SW1 is mapped to Port D
 */
void __attribute__ ((interrupt)) HMI_ISR(void);

#endif /* HMI_H_ */
