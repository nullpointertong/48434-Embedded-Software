/*! @file
 *
 *  @brief Implmentation for DEM Calculations
 *
 *  This contains the methods required in order to do DEM calulcations.
 *
 *  @author 12876417
 *  @date 2019-10-22
 */

#ifndef DEMCALCULATIONS_H
#define DEMCALCULATIONS_H

#include "analog.h"
#include "Cpu.h"
#include "packet.h"
#include <math.h>

// Simple OS library
#include "OS.h"

//Custom Round Macro for rounding numbers for Q notation
#define DEM_ROUND(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

//Defining Command bits for the "basic" functionality
#define TOWER_TEST_MODE_COMMAND_BIT 0xA
#define TOWER_TARIFF_COMMAND_BIT 0xB
#define TOWER_TIME1_COMMAND_BIT 0xC
#define TOWER_TIME2_COMMAND_BIT 0xD
#define TOWER_POWER_COMMAND_BIT 0xE
#define TOWER_ENERGY_COMMAND_BIT 0xF
#define TOWER_COST_COMMAND_BIT 0x10

//Defining Command bits for the "Intermediate" functionality
#define TOWER_FREQUENCY_COMMAND_BIT 0x11
#define TOWER_VOLTAGE_COMMAND_BIT 0x12
#define TOWER_CURRENT_COMMAND_BIT 0x13
#define TOWER_POWERFACTOR_COMMAND_BIT 0x1A

//In cents
#define TOWER_TARIFF1_PEAK 22.235
#define TOWER_TARIFF1_SHOULDER 4.400
#define TOWER_TARIFF1_OFFPEAK 2.109
#define TOWER_TARIFF2 1.713
#define TOWER_TARIFF3 4.100

//Used for Radian Calculations
#define DEM_PI 3.14159


//Extern customized cos and sin
extern float  DEM_CustomCos[256];
extern float  DEM_CustomSin[256];

//Total Energy/Cost in cents and kW
extern uint64_t       DEM_CostTotal;
extern uint64union_t  DEM_EnergyTotal;

//Voltage and Sin circular array
extern int16_t  DEM_VoltageSin[16],  DEM_CurrentSin[16];

//Select Tariff Rate in cents
static volatile uint8_t DEM_TariffSelect;

//Power in kWh
extern uint64union_t DEM_Power;

//Time in ns
extern uint64_t DEM_Timer;

//Global Variables to update the RMS
extern uint32union_t DEM_RMSVoltage;
extern uint32union_t DEM_RMSCurrent;
extern uint32union_t DEM_PowerFactor;

//Time in Seconds for the tower
extern uint64_t DEM_TimeSeconds;

//Global Index
extern uint8_t DEM_IndexHead;

extern uint16_t DEM_PeriodSample[2];

extern uint32_t DEM_TimeArray[2];

//Global Test Bool
extern bool DEM_TestModeBool;


/*! @brief Calculates the energy in a period
 *  @param Voltage Array is an array containing instantaneous voltage values
 *  Current Array is an array containing instantaneous current values
 *  samples number of samples in the array
 *
 *  @return int32_t - Energy in kWh in 32Q16
 */
int32_t DEM_CalculateEnergy (const int16_t voltageArray[], const int16_t currentArray[], const uint8_t samples);

/*! @brief Calculates Frequency and Phase Angle
 *  @param cycleStatus is the status of the cycle when it stops and starts
 *  frequency is the local frequnecy that updates the global frequency
 *  index is the array/sample index
 *  sampleSize number of samples in the array
 *
 *  @return bool - True if calculation is sucessful
 */
bool DEM_CalculateFrequencyAndPhase(bool * cycleStatus, uint16_t * frequency, uint16_t * phaseAngle, const uint8_t index, uint8_t * sampleSize);

/*! @brief Process the Power using Power Factor, Voltage and Current
 *  @param frequency is the local frequency that updates the global frequency
 *  phaseAngle is the phase angle
 *  index is the index of the current and voltage arrays
 *  sampleSize number of samples in the array
 *  @return bool - True if calculation is successful
 */
bool DEM_CalculatePower();

/*! @brief Calculates Cost
 *
 *  @return bool - True if calculation is successful
 */
bool DEM_CalculateCost();

/*! @brief This function Calculates RMS
 *
 *  @return int32_t - RMS in 32Q16
 */
uint32_t DEM_CalculateRMS(const int16_t * values, const uint8_t sampleSize);

/*! @brief This function Calculates the square root using a binary search pattern
 *  @param number to be rooted
 *
 *  @return uint32_t - decimal root in 32Q16
 */
uint32_t DEM_Root (float number);

/*! @brief This function converts a float into a fixed number
 *  in 32Q16 format
 *  @param floatingPoint number to be converted into 32Q16
 *
 *  @return uint32_t - the fixed point number
 */
uint32_t DEM_ConvertTo32Q16(float floatingPoint);

/*! @brief This function converts a fixed number into a float
 *  from 32Q16
 *  @param fixedPoint number to be converted into float from 32Q16
 *
 *  @return float - the float
 */
float DEM_ConvertFrom32Q16(uint32_t fixedPoint);

/*! @brief This function converts a fixed number into a float
 *  from 8Q3
 *  @param floatingPoint number to be converted into 8Q3
 *  Note: This is primarily used for compacting variables for flash storage
 *  @return uint8_t - the float
 */
uint8_t DEM_ConvertTo8Q3(float floatingPoint);

/*! @brief This function converts a fixed number into a float
 *  from 8Q3
 *  @param fixedPoint number to be converted into float from 8Q3
 *  Note: This is primarily used for compacting variables for flash storage
 *  @return float - the float
 */
float DEM_ConvertFrom8Q3(uint8_t fixedPoint);

/*! @brief This function converts a fixed number into a float
 *  @param floatingPoint number to be converted to custom Q format
 *  qpoint number of points to be shifted by
 *  Note: This is a universal Q conversion and thus will require a cast if number isn't a 64 bit int
 *  @return int64_t - the float
 */
int64_t DEM_ConvertToQ(float floatingPoint, uint8_t qpoint);

/*! @brief This function converts a fixed number into a float
 *  @param fixedPoint number to be converted into float
 *  qpoint number of points to be shifted by
 *  Note: This is a universal Q conversion and thus will require a cast if number isn't a 64 bit int
 *  @return float - the float
 */
float DEM_ConvertFromQ(int64_t fixedPoint, uint8_t qpoint);


/*! @brief This function efficiently divides by a factor of 10
 *  @param dividend number to be divided
 *  factors by which power of 10 up to 3
 *
 *  @return int32_t - the number divided by 10,100,1000 returns 0 if invalid (i.e factor not a factor of 10
 */
int32_t DEM_Efficent32Div10(int32_t dividend , int8_t factors);


#endif
