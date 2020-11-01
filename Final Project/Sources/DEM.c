/*!
** @file demCalculations.c
** @brief DEM Calculations
**
**
**
** @author 12876417
** @date 2019-10-22
**
*/

//TODO: Naming convention for function
#include "DEM.h"

//Total Cost in cents 
uint64_t DEM_CostTotal;
//Circular Arrays used for storing samples of the Current and Sin wave 16Q8
int16_t  DEM_VoltageSin[16],  DEM_CurrentSin[16];
//Power in kWh
uint64union_t  DEM_Power;
//Timer in period for 1 cycle 
uint64_t  DEM_Timer;

//Energy,RMS Voltage/Current,Power Factor
uint64union_t  DEM_EnergyTotal;
uint32union_t  DEM_RMSVoltage;
uint32union_t  DEM_RMSCurrent;
uint32union_t  DEM_PowerFactor;


//Time in Seconds
uint64_t  DEM_TimeSeconds;
//Global Index to track number of samples
uint8_t  DEM_IndexHead;
//Period of Samples
uint16_t  DEM_PeriodSample[2];
//Time Stamp for when the samples are collected 
uint32_t  DEM_TimeArray [2];

//Bool to signal for test Mode
bool DEM_TestModeBool;

//Custom Look up Tables for Cos
float DEM_CustomCos[256] = {
    1,0.999847695,0.999390827,0.998629535,0.99756405,0.996194698,0.994521895,0.992546152,0.990268069,0.987688341,0.984807753,0.981627183
    ,0.978147601,0.974370065,0.970295726,0.965925826,0.961261696,0.956304756,0.951056516,0.945518576,0.939692621,0.933580426,0.927183855
    ,0.920504853,0.913545458,0.906307787,0.898794046,0.891006524,0.882947593,0.874619707,0.866025404,0.857167301,0.848048096,0.838670568
    ,0.829037573,0.819152044,0.809016994,0.79863551,0.788010754,0.777145961,0.766044443,0.75470958,0.743144825,0.731353702,0.7193398
    ,0.707106781,0.69465837,0.68199836,0.669130606,0.656059029,0.64278761,0.629320391,0.615661475,0.601815023,0.587785252,0.573576436
    ,0.559192903,0.544639035,0.529919264,0.515038075,0.5,0.48480962,0.469471563,0.4539905,0.438371147,0.422618262,0.406736643,0.390731128
    ,0.374606593,0.35836795,0.342020143,0.325568154,0.309016994,0.292371705,0.275637356,0.258819045,0.241921896,0.224951054,0.207911691
    ,0.190808995,0.173648178,0.156434465,0.139173101,0.121869343,0.104528463,0.087155743,0.069756474,0.052335956,0.034899497,0.017452406
    ,6.12574E-17,-0.017452406,-0.034899497,-0.052335956,-0.069756474,-0.087155743,-0.104528463,-0.121869343,-0.139173101,-0.156434465
    ,-0.173648178,-0.190808995,-0.207911691,-0.224951054,-0.241921896,-0.258819045,-0.275637356,-0.292371705,-0.309016994,-0.325568154
    ,-0.342020143,-0.35836795,-0.374606593,-0.390731128,-0.406736643,-0.422618262,-0.438371147,-0.4539905,-0.469471563,-0.48480962,-0.5
    ,-0.515038075,-0.529919264,-0.544639035,-0.559192903,-0.573576436,-0.587785252,-0.601815023,-0.615661475,-0.629320391,-0.64278761
    ,-0.656059029,-0.669130606,-0.68199836,-0.69465837,-0.707106781,-0.7193398,-0.731353702,-0.743144825,-0.75470958,-0.766044443
    ,-0.777145961,-0.788010754,-0.79863551,-0.809016994,-0.819152044,-0.829037573,-0.838670568,-0.848048096,-0.857167301,-0.866025404
    ,-0.874619707,-0.882947593,-0.891006524,-0.898794046,-0.906307787,-0.913545458,-0.920504853,-0.927183855,-0.933580426,-0.939692621
    ,-0.945518576,-0.951056516,-0.956304756,-0.961261696,-0.965925826,-0.970295726,-0.974370065,-0.978147601,-0.981627183,-0.984807753
    ,-0.987688341,-0.990268069,-0.992546152,-0.994521895,-0.996194698,-0.99756405,-0.998629535,-0.999390827,-0.999847695,-1,-0.999847695
    ,-0.999390827,-0.998629535,-0.99756405,-0.996194698,-0.994521895,-0.992546152,-0.990268069,-0.987688341,-0.984807753,-0.981627183
    ,-0.978147601,-0.974370065,-0.970295726,-0.965925826,-0.961261696,-0.956304756,-0.951056516,-0.945518576,-0.939692621,-0.933580426
    ,-0.927183855,-0.920504853,-0.913545458,-0.906307787,-0.898794046,-0.891006524,-0.882947593,-0.874619707,-0.866025404,-0.857167301
    ,-0.848048096,-0.838670568,-0.829037573,-0.819152044,-0.809016994,-0.79863551,-0.788010754,-0.777145961,-0.766044443,-0.75470958
    ,-0.743144825,-0.731353702,-0.7193398,-0.707106781,-0.69465837,-0.68199836,-0.669130606,-0.656059029,-0.64278761,-0.629320391
    ,-0.615661475,-0.601815023,-0.587785252,-0.573576436,-0.559192903,-0.544639035,-0.529919264,-0.515038075,-0.5,-0.48480962
    ,-0.469471563,-0.4539905,-0.438371147,-0.422618262,-0.406736643,-0.390731128,-0.374606593,-0.35836795,-0.342020143,-0.325568154
    ,-0.309016994,-0.292371705,-0.275637356,-0.258819045
};

int32_t DEM_CalculateEnergy (const int16_t voltageArray[], const int16_t currentArray[], const uint8_t samples)
{
  //Sum of Energy in 1 Period
  uint32_t energySum = 0;

  //Iterate through the voltage and current values
  for (uint8_t i = 0; i< samples; i++)
    energySum += voltageArray[i] * currentArray[i];
  if(!DEM_TestModeBool)
    //Converted from 16 bit into 32Q16 in kWh by dividing by 3600000 and multiplying by 100 the factor of input to output
    return DEM_ConvertToQ((float)(DEM_ConvertFromQ(energySum,16) / 36000) , 16);
  else if(DEM_TestModeBool)
    return DEM_ConvertToQ((float)(DEM_ConvertFromQ(energySum,16) / 10) , 16);
}


bool DEM_CalculateFrequencyAndPhase(bool * cycleStatus, uint16_t * frequency, uint16_t * phaseAngle, const uint8_t index, uint8_t * sampleSize )
{
   //Variables for storing the time when
  static uint8_t timeDifference[2] = {0};
  static uint16_t zeroVoltTime, zeroCurrentTime;

  //Find the
  uint8_t oldValueIndex = index-1;
  if (index == 0)
    oldValueIndex = 15;

  OS_DisableInterrupts();
  //Check for Zero Crossing for voltage
  if ((DEM_VoltageSin[oldValueIndex] <= 0) && (DEM_VoltageSin[index] >= 0)&&(*sampleSize >= 16))
  {
    //Cycle Starting
    *cycleStatus = true;

    uint16_t period = (DEM_PeriodSample[0] - timeDifference[0]) + timeDifference[1] + (DEM_PeriodSample[1] * (*sampleSize));

    //Linear Approximation for voltage
    zeroVoltTime = DEM_TimeArray[0] - (DEM_VoltageSin[oldValueIndex] * (DEM_TimeArray[1] - DEM_TimeArray[0]) /(DEM_VoltageSin[index] -
    DEM_VoltageSin[oldValueIndex]));

    OS_EnableInterrupts();

    //Get the local frequency based of the period
    uint16_t frequencyLocal = 1000000000 / period;

    //Keep Old Time Difference
    timeDifference[0] = timeDifference[1];
    //Get the new Time Difference
    timeDifference[1] = DEM_TimeArray[1] - zeroVoltTime;

    //Check that clocks are within provided range
    if ((frequencyLocal>= 47500)&&(frequencyLocal<= 52500))
    {
      DEM_PeriodSample[0] = DEM_PeriodSample[1];
      DEM_PeriodSample[1] = period/16;
      *frequency = frequencyLocal;
    }
  }
  else
    ++(*sampleSize);

  OS_DisableInterrupts();
  //Check for Zero crossing in Current
  if ((DEM_CurrentSin[oldValueIndex] <=0) && (DEM_CurrentSin[index] >= 0))
  {
    //Find zero crossing point using Linear Approximation
    zeroCurrentTime = DEM_TimeArray[0] - (DEM_CurrentSin[oldValueIndex] * (DEM_TimeArray[1] - DEM_TimeArray[0])
    /(DEM_CurrentSin[index] - DEM_CurrentSin[oldValueIndex]));

    OS_EnableInterrupts();
    //Calculate Period
    uint16_t period = 1000000000/ (*frequency);

    //Get Phase Angle
    phaseAngle = ((zeroVoltTime - zeroCurrentTime)/period)*360;
  }

  return true;
}

bool DEM_CalculatePower ()
{
  OS_DisableInterrupts();
  //Calculate Power using Power Factor and RMS Volts/Current all in 64Q16
  DEM_Power.l = (DEM_PowerFactor.l*DEM_RMSVoltage.l*DEM_RMSCurrent.l)>>32;
  OS_EnableInterrupts();

  return true;  
}

bool DEM_CalculateCost (const int32_t EnergyInPeriod)
{
  //Amount of hours directly taken from RTC
  uint8_t hours =  DEM_TimeSeconds / 3600;

  //Time taken from DEM for peak cost
  uint8_t time = (DEM_TimeSeconds % 86400)/3600;

  //Cost in cents
  uint32_t cost;

  //Calculate Tariff Based on Tariff Select
  if (DEM_TariffSelect == 1)
  {
    //Check time and apply correct Tariff
    if (time >= 14 || time <= 20)
    {
      cost =  EnergyInPeriod *  hours * TOWER_TARIFF1_PEAK * 100;
    }
    else if (time >= 7 || time < 14 && time > 20 || time <= 22)
    {
      cost = EnergyInPeriod * hours *  TOWER_TARIFF1_SHOULDER * 100;
    }
    else
    {
      cost = EnergyInPeriod * hours *  TOWER_TARIFF1_OFFPEAK * 100;
    }
  }
  else if (DEM_TariffSelect == 2)
  {
    cost = EnergyInPeriod * hours *  TOWER_TARIFF2 * 100;
  }
  else if (DEM_TariffSelect == 3)
  {
    cost = EnergyInPeriod * hours * TOWER_TARIFF3 * 100;
  }

  OS_DisableInterrupts();
  //Add to total
  DEM_CostTotal += cost;
  OS_EnableInterrupts();

  return true;
}

uint32_t DEM_CalculateRMS(const int16_t values[], const uint8_t sampleSize)
{
  uint32_t sum = 0;

  //Square the value
  for(uint8_t i = 0; i < sampleSize; i++)
    sum += values[i] * values[i];

  //Set the RMS into a 32Q16 Number
  return (uint32_t)DEM_Root(DEM_ConvertFromQ(sum,16) / sampleSize);
}

uint32_t DEM_Root(float number)
{
  //Using the binary search pattern we take a low,mid and high
  float low = 0;
  float high = number;
  float middle;

  for (uint16_t i = 0 ; i < 1000 ; i++)
  {
      //Take half of Low and High
      middle = (low+high)/2;

      //Do a binary search checking if we get the value, overshoot or undershoot
      if (middle*middle == number)
	return middle;
      if (middle*middle > number)
	high = middle;
      else
	low = middle;
  }
  //Return Value in 32Q16
  return (uint32_t)DEM_ConvertToQ(middle,16);
}

inline uint32_t DEM_ConvertTo32Q16(float floatingPoint)
{
  return (uint32_t)(DEM_ROUND(floatingPoint * (1 << 16)));
}

inline float DEM_ConvertFrom32Q16(uint32_t fixedPoint)
{
  return ((float)fixedPoint / (float)(1 << 16));
}

//TODO: Research the from method
inline uint8_t DEM_ConvertTo8Q3(float floatingPoint)
{
  return (uint8_t)(DEM_ROUND(floatingPoint * (1 << 3)));
}

inline float DEM_ConvertFrom8Q3(uint8_t fixedPoint)
{
  return ((float)fixedPoint / (float)(1 << 3));
}

inline int64_t DEM_ConvertToQ(float floatingPoint, uint8_t qpoint)
{
  return (int64_t)(DEM_ROUND(floatingPoint * (1 << qpoint)));
}

inline float DEM_ConvertFromQ(int64_t fixedPoint, uint8_t qpoint)
{
  return ((float)fixedPoint / (float)(1 << qpoint));
}

inline int32_t DEM_Efficent32Div10(int32_t dividend , int8_t factors)
{
  int64_t bitDivisor; 
   switch(factors)
   {
     case (1):
     //bitDivisor = 2^32 * 1/10
     bitDivisor = 0x1999999A;
     //Remove the 2^32
     return (int32_t) ((bitDivisor * dividend) >> 32);
     break;

     case (2):
     //bitDivisor = 2^32 * 1/100
     bitDivisor = 0x28F5C29;
     //Remove the 2^32
     return (int32_t) ((bitDivisor * dividend) >> 32);
     break;

     case (3):
     //bitDivisor = 2^32 * 1/100
     bitDivisor = 0x1999999A;
     //Remove the 2^32
     return (int32_t) ((bitDivisor * dividend) >> 32);
     break;
   }
}

/*!
* @}
*/
