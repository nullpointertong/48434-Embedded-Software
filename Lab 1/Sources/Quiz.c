//2019 Autumn
//8N1 BR:500000 PTE1 = INPUT, PTE0 = OUTPUT UART2_TX 

#include "Cpu.h"

// ----------------------------------------
// CTS latency
const uint8_t CTS_OFF_LATENCY = 4;
const uint8_t CTS_ON_LATENCY = 32;
// ----------------------------------------
// FIFO structure
typedef struct
{
 uint16_t start, end;
 uint16_t volatile nbBytes;
 uint8_t buffer[FIFO_SIZE];
} TFIFO;
/*! @brief Initialize the FIFO before first use.
*
* @param aFIFO A pointer to the FIFO that needs initializing.
* @return void
*/
void FIFO_Init(TFIFO* const aFIFO);
/*! @brief Put one character into the FIFO.
*
* @param aFIFO A pointer to a FIFO where data is to be stored.
* @param data A byte of data to store in the FIFO buffer.
* @return true if successful, false if unsuccessful.
* @note Assumes that FIFO_Init has been called.
*/
bool FIFO_Put(TFIFO* const aFIFO, const uint8_t data);
/*! @brief Get one character from the FIFO.
*
* @param aFIFO A pointer to a FIFO with data to be retrieved.
* @param dataPtr A memory location to place the retrieved byte.
* @return true if successful, false if unsuccessful.
* @note Assumes that FIFO_Init has been called.
*/
bool FIFO_Get(TFIFO* const aFIFO, uint8_t* const dataPtr);
/*! @brief Reports on the amount of free space in the FIFO.
*
* @param aFIFO A pointer to a FIFO to interrogate.
* @return The number of free spaces in the FIFO.
* @note Assumes that FIFO_Init has been called.
*/
uint16_t FIFO_Space(TFIFO* const aFIFO);


// Specify global variables
static TFIFO* RxFIFO,TxFIFO; 

/*! @brief Initialize the UART2 interface.
*
* @return void
*/
void UART2_Init(void)
{
  __DI();

  //25000000/16*38400 = 40.69010 
  //Set High, Low, BRFA
  UART2_BDH = 0;
  UART2_BDL = 40;
  UART2_C4  = 22;

  // For the UART initialization, you may hard-code values directly into the C2,
  UART2_C2 |= UART_C2_RIE_MASK | UART_C2_TE_MASK | UART_C2_RE_MASK;

  // NVIC registers to enable UART interrupts. IRQ =49
  NVICICPR1 = (1<<17);
  NVICISER1 = (1<<17);

 
  //CTS On 
  GPIOA_PDDR = 1;
  //Set up PTA0 as Output and as 0
  GPIOA_PCOR = 1; 
   


  // Show any global data structures required.
  FIFO_Init(RxFIFO);
  FIFO_Init(TxFIFO);
  
  __EI();
}



/*! @brief Get a character from the receive FIFO if it is not empty.
*
* @param dataPtr is a pointer to memory to store the retrieved byte.
* @return true if successful, false if unsuccessful.
* @note Assumes the receive FIFO has been initialized.
*/
bool UART2_InChar(uint8_t* const dataPtr)
{
  bool s; 
  //Critical Selection
  UART2_C2 &= ~ UART_C2_RIE_MASK;
  
  s = FIFO_Get(RxFIFO, dataPtr);

  UART2_C2 |= UART_C2_RIE_MASK;

  return s; 
}


/*! @brief Put a byte in the transmit FIFO if it is not full.
*
* @param data is a byte to be placed in the transmit FIFO.
* @return true if successful, false if unsuccessful.
* @note Assumes the receive FIFO has been initialized.
*/
bool UART2_OutChar(const uint8_t data)
{
  bool s; 
  //Critical Selection
  UART2_C2 &= ~ UART_C2_TIE_MASK;

  //Put a byte in FIFO Transmit
  s = FIFO_Put(TxFIFO, data);
  
  UART2_C2 |= UART_C2_TIE_MASK;

  return s;
}


// Write the UART2 interrupt handler that performs the actual serial I/O
// operations. Show how you need to modify vectors.c.
(tIsrFunc)&UART2_ISR, /* UART2_RX_TX*/


void __attribute__ ((interrupt)) UART2_ISR()
{
    if (UART2_C2 & UART_C2_RIE_MASK)
    {
        if (UART2_S1 & UART_S1_RDRF_MASK)
        {
            if (!GPIOA_PDOR)
            {
                if (FIFO_Put(RxFIFO,(uint8_t) UART2_D) && FIFO_Space(RxFIFO) > 4)
                {
                  GPIOA_PDOR = 0;
                }
                else if (FIFO_Put(RxFIFO,(uint8_t) UART2_D) && FIFO_Space(RxFIFO) > 0)
                {
                  GPIOA_PDOR = 1; 
                }
            }
        }
    }

    if (UART2_C2 & UART_C2_TIE_MASK)
    {
        if (UART2_S1 & UART_S1_TDRE_MASK)
        {
            while(!FIFO_Put(TxFIFO, &UART2_D));
            UART2_C2 &= ~UART_C2_TIE_MASK
        }
    }

// 1. K70 initially asserts /CTS (PTA0 = 0), because it’s Receive FIFO is empty.
// 2. K70 receives multiple 10-bit serial frames on TxD (UART2_RX).
// 3. K70 sets /CTS high (PTA0 = 1) when it’s Receive FIFO becomes nearly full.
// The FT232BM may transmit up to 4 more bytes once /CTS is high.
// 4. K70 asserts /CTS (PTA0 = 0) because there is sufficient space in the Receive
// FIFO to receive more bytes from the FT232BM.
}


#define FIFO_SIZE 512 // Size of FIFO
static uint8_t PutI; // Index of where to put next
static uint8_t GetI; // Index of where to get next
static uint16_t Size; // Number of elements currently in the FIFO
// The statically stored FIFO data
static uint8_t FIFO[FIFO_SIZE / 2];
static const uint8_t LO_NIBBLE_MASK = 0x0F;
static const uint8_t HI_NIBBLE_MASK = 0xF0;

/*! @brief Initialize the FIFO.
*
* @return void
* @note May be called at any time (i.e. reinitialized).
*/
void FIFO_Init(void)
{
  PutI = 0;
  GetI = 0; 
  Size = 0;
}

/*! @brief Put data into the FIFO.
*
* @param data holds the 4-bit data (in the lower nibble) to store.
* @return true if successful, false if unsuccessful
* because FIFO was full.
* @note Assumes FIFO_Init has been called.
*/
bool FIFO_Put(const uint8_t data)
{
  static bool lowNibble = true;

  if (!data)
    return false;
  
  if (lowNibble)
  {
    FIFO[PutI] &= ~LO_NIBBLE_MASK; 
    FIFO[PutI] |= data & LO_NIBBLE_MASK; 
    lowNibble = false;
  }
  else if (!lowNibble)
  {
    FIFO[PutI] &= ~HI_NIBBLE_MASK; 
    FIFO[PutI] |= data<<4 & HI_NIBBLE_MASK; 
    lowNibble = true;
    PutI++;
  }

  if (FIFO[PutI] == FIFO[FIFO_SIZE]) 
    PutI = 0;
  
  return true;
}

/*! @brief Get data from the FIFO.
*
* @param dataPtr is a pointer to where the 4-bit data from the FIFO
* is to be stored (in the lower nibble).
* @return true if successful, false if unsuccessful
* because FIFO was empty.
* @note Assumes FIFO_Init has been called.
*/
bool FIFO_Get(uint8_t* const dataPtr)
{
  static bool lowNibble = true;

  if (!dataPtr || *dataPtr == 0)
    return false;
  
  if (lowNibble)
  {
    *dataPtr = FIFO[GetI] & LO_NIBBLE_MASK; 
    lowNibble = false;
  }
  else if (!lowNibble)
  {
    *dataPtr = (FIFO[GetI] & HI_NIBBLE_MASK)>>4;
    lowNibble = true;
    GetI++;
  }

  if (FIFO[GetI] == FIFO[FIFO_SIZE]) 
    PutI = 0;
  
  return true;
}

//3. 
// In addition to the three public global variables, specifiy any additional

// Execution times in microseconds
uint16_t Debug_Minimum, Debug_Maximum, Debug_Average;

// private global variables you will require.
uint16_t static Sum, Count;


void Debug_Init()
{
  __DI();
  // Write the Debug_Init() function that initializes the global variables 
  Sum           = 0;
  Count         = 0;
  Debug_Minimum = 0xFFFF;
  Debug_Maximum = 0;
  Debug_Average = 0;

  // the FTM0 timer system as a free-running 16-bit counter only. The clock
  // source for FTM0 should be the external clock (EXTCLK). 
  SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;

  //CNSC,CNTIN, MOD, CNT, SC, MODE
  FTM0_CnSC(0) =  FTM_CnSC_MSA_MASK;

  FTM0_CNTIN   =  0x00;
  FTM0_MOD     =  0xFFFF;
  FTM0_CNT     =  0x00;

  FTM0_SC      =  FTM_SC_CLKS(3);
  FTM0_CnSC(0) =  FTM_CnSC_CHIE_MASK;

  FTM0_MODE    = FTM_MODE_FTMEN_MASK;

  //NVIC 62
  NVICICPR1 = (1<<30);
  NVICISER1 = (1<<30);

  __EI();
}

void Debug_Begin()
{
  FTM0_CNT = 0; 
}  

void Debug_End()
{
    if(FTM0_CNT > Debug_Maximum)
    {
      Debug_Maximum = FTM0_CNT;
    }
    if(FTM0_CNT < Debug_Minimum)
    {
      Debug_Minimum = FTM0_CNT;
    }
    Sum += FTM0_CNT;
    Count++;

    Debug_Average = Sum/Count;
    FTM0_CNT = 0;
}