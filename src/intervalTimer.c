#include "intervalTimer.h"
#include "supportFiles/display.h"
#include "xparameters.h"
#include "supportFiles/utils.h"

#include <stdint.h>
#include <stdio.h>
#include <xil_io.h>

// **SIMPLE USAGE:**
//	intervalTimer_init(0);
//	intervalTimer_reset(0);
//	intervalTimer_start(0);
//	***TEST CODE GOES HERE***
//	intervalTimer_stop(0);
//	double seconds;
//	intervalTimer_getTotalDurationInSeconds(0, &seconds);
//	printf("Time: %f ms", seconds * 1000);

// //380 ns for short function call


// Interval timer parameters
#define INTERVAL_TIMER_0_BASEADDR XPAR_AXI_TIMER_0_BASEADDR
#define INTERVAL_TIMER_1_BASEADDR XPAR_AXI_TIMER_1_BASEADDR
#define INTERVAL_TIMER_2_BASEADDR XPAR_AXI_TIMER_2_BASEADDR

#define INTERVAL_TIMER_0_CLOCK_FREQ_HZ XPAR_AXI_TIMER_0_CLOCK_FREQ_HZ
#define INTERVAL_TIMER_1_CLOCK_FREQ_HZ XPAR_AXI_TIMER_1_CLOCK_FREQ_HZ
#define INTERVAL_TIMER_2_CLOCK_FREQ_HZ XPAR_AXI_TIMER_2_CLOCK_FREQ_HZ

// Additional Xilinx timer specifications
#define INTERVAL_TIMER_TCSR0_OFFSET 0x00
#define INTERVAL_TIMER_TLR0_OFFSET 0x04
#define INTERVAL_TIMER_TCR0_OFFSET 0x08
#define INTERVAL_TIMER_TCSR1_OFFSET 0x10
#define INTERVAL_TIMER_TLR1_OFFSET 0x14
#define INTERVAL_TIMER_TCR1_OFFSET 0x18

#define INTERVAL_TIMER_TCSR_MDT_BIT 0
#define INTERVAL_TIMER_TCSR_UDT_BIT 1
#define INTERVAL_TIMER_TCSR_GENT_BIT 2
#define INTERVAL_TIMER_TCSR_CAPT_BIT 3
#define INTERVAL_TIMER_TCSR_AFHT_BIT 4 // ARHT on TCSR1
#define INTERVAL_TIMER_TCSR_LOAD_BIT 5
#define INTERVAL_TIMER_TCSR_ENIT_BIT 6
#define INTERVAL_TIMER_TCSR_ENT_BIT 7
#define INTERVAL_TIMER_TCSR_TINT_BIT 8
#define INTERVAL_TIMER_TCSR_PWMA_BIT 9 // PWMo on TCSR1
#define INTERVAL_TIMER_TCSR_ENALL_BIT 10
#define INTERVAL_TIMER_TCSR_CASC_BIT 11 // Note: Only TCSR0 has a CASC bit.

#define INTERVAL_TIMER_TCR_WIDTH 32

// Other constants
#define INTERVAL_TIMER_SHORT_MS_DELAY 10
#define INTERVAL_TIMER_SINGLE_BIT 0x1

#define INTERVAL_TIMER_0_ID 0
#define INTERVAL_TIMER_1_ID 1
#define INTERVAL_TIMER_2_ID 2

// Internal functions prototypes
uint64_t intervalTimer_getTotalDurationInTicks(uint32_t timerNumber);
int32_t intervalTimer_readTimerRegister(uint32_t timerNumber, int32_t offset);
void intervalTimer_writeTimerRegister(uint32_t timerNumber, int32_t offset, int32_t value);
void intervalTimer_writeTimerRegisterBit(uint32_t timerNumber, int32_t offset, int32_t k, int32_t b);
int32_t intervalTimer_readTimerRegisterBit(uint32_t timerNumber, int32_t offset, int32_t k, int32_t b);
int32_t intervalTimer_getBit(int32_t x, int32_t k);
int32_t intervalTimer_setBit(int32_t x, int32_t k, int32_t b);
int32_t intervalTimer_getTimerFrequency(uint32_t timerNumber);
uint8_t intervalTimer_CheckValidTimerAndInitialized(uint32_t timerNumber);
int32_t intervalTimer_getTimerBaseAddress(uint32_t timerNumber);

// Global variables
uint8_t intervalTimer_initializeFlag[3] = {INTERVAL_TIMER_STATUS_FAIL, INTERVAL_TIMER_STATUS_FAIL, INTERVAL_TIMER_STATUS_FAIL};

// Start timer. Init must be called beforehand or timer will display an error.
uint32_t intervalTimer_start(uint32_t timerNumber)
{
	if(intervalTimer_CheckValidTimerAndInitialized(timerNumber) == INTERVAL_TIMER_STATUS_FAIL)
		return INTERVAL_TIMER_STATUS_FAIL;
	intervalTimer_writeTimerRegisterBit(timerNumber, INTERVAL_TIMER_TCSR0_OFFSET, INTERVAL_TIMER_TCSR_ENT_BIT, 1);
	return INTERVAL_TIMER_STATUS_OK;
}

// Pause timer.
uint32_t intervalTimer_stop(uint32_t timerNumber)
{
	if(intervalTimer_CheckValidTimerAndInitialized(timerNumber) == INTERVAL_TIMER_STATUS_FAIL)
		return INTERVAL_TIMER_STATUS_FAIL;
	intervalTimer_writeTimerRegisterBit(timerNumber, INTERVAL_TIMER_TCSR0_OFFSET, INTERVAL_TIMER_TCSR_ENT_BIT, 0);
	return INTERVAL_TIMER_STATUS_OK;
}

// Load a zero into timer. Does not stop timer if running.
uint32_t intervalTimer_reset(uint32_t timerNumber)
{
	if(intervalTimer_CheckValidTimerAndInitialized(timerNumber) == INTERVAL_TIMER_STATUS_FAIL)
		return INTERVAL_TIMER_STATUS_FAIL;
	// Load a zero into timer.
	intervalTimer_writeTimerRegister(timerNumber, INTERVAL_TIMER_TLR0_OFFSET, 0);
	intervalTimer_writeTimerRegister(timerNumber, INTERVAL_TIMER_TLR1_OFFSET, 0);
	intervalTimer_writeTimerRegisterBit(timerNumber, INTERVAL_TIMER_TCSR0_OFFSET, INTERVAL_TIMER_TCSR_LOAD_BIT, 1);
	intervalTimer_writeTimerRegisterBit(timerNumber, INTERVAL_TIMER_TCSR1_OFFSET, INTERVAL_TIMER_TCSR_LOAD_BIT, 1);
	intervalTimer_writeTimerRegisterBit(timerNumber, INTERVAL_TIMER_TCSR0_OFFSET, INTERVAL_TIMER_TCSR_LOAD_BIT, 0);
	intervalTimer_writeTimerRegisterBit(timerNumber, INTERVAL_TIMER_TCSR1_OFFSET, INTERVAL_TIMER_TCSR_LOAD_BIT, 0);
	return INTERVAL_TIMER_STATUS_OK;
}

// Initialize timer. Load a zero into timer and set to count up.
uint32_t intervalTimer_init(uint32_t timerNumber)
{
	if(intervalTimer_initializeFlag[timerNumber] == INTERVAL_TIMER_STATUS_OK)
	{
		printf("Timer %d already initialized.\r\n", (int)timerNumber);
		return INTERVAL_TIMER_STATUS_OK;
	}
	// Load zero into timer.
	intervalTimer_writeTimerRegister(timerNumber, INTERVAL_TIMER_TLR0_OFFSET, 0);
	intervalTimer_writeTimerRegister(timerNumber, INTERVAL_TIMER_TLR1_OFFSET, 0);
	intervalTimer_writeTimerRegisterBit(timerNumber, INTERVAL_TIMER_TCSR0_OFFSET, INTERVAL_TIMER_TCSR_LOAD_BIT, 1);
	intervalTimer_writeTimerRegisterBit(timerNumber, INTERVAL_TIMER_TCSR1_OFFSET, INTERVAL_TIMER_TCSR_LOAD_BIT, 1);

	// Reset control registers.
	intervalTimer_writeTimerRegister(timerNumber, INTERVAL_TIMER_TCSR0_OFFSET, 0);
	intervalTimer_writeTimerRegister(timerNumber, INTERVAL_TIMER_TCSR1_OFFSET, 0);

	// Cascade counters and count up.
	intervalTimer_writeTimerRegisterBit(timerNumber, INTERVAL_TIMER_TCSR0_OFFSET, INTERVAL_TIMER_TCSR_CASC_BIT, 1);
	intervalTimer_writeTimerRegisterBit(timerNumber, INTERVAL_TIMER_TCSR0_OFFSET, INTERVAL_TIMER_TCSR_UDT_BIT, 0);

	// Double check to make sure registers show correct value.
	intervalTimer_initializeFlag[timerNumber] =
		((intervalTimer_readTimerRegister(timerNumber, INTERVAL_TIMER_TCSR0_OFFSET) == (INTERVAL_TIMER_SINGLE_BIT << INTERVAL_TIMER_TCSR_CASC_BIT))
		&& (intervalTimer_readTimerRegister(timerNumber, INTERVAL_TIMER_TCSR1_OFFSET) == 0)
		&& (intervalTimer_readTimerRegister(timerNumber, INTERVAL_TIMER_TCR0_OFFSET) == 0)
		&& (intervalTimer_readTimerRegister(timerNumber, INTERVAL_TIMER_TCR1_OFFSET) == 0))
		? INTERVAL_TIMER_STATUS_OK : INTERVAL_TIMER_STATUS_FAIL;
	if (intervalTimer_initializeFlag[timerNumber] == INTERVAL_TIMER_STATUS_FAIL)
	{
		printf("Error. Timer %d Initialization failed.\r\n", (int)timerNumber);
	}
	return intervalTimer_initializeFlag[timerNumber];
}

// Initialize all 3 timers.
uint32_t intervalTimer_initAll()
{
	for(uint8_t timerNumber = 0; timerNumber < INTERVAL_TIMER_NUMBER_OF_TIMERS; timerNumber++)
	{
		if(intervalTimer_init(timerNumber) == INTERVAL_TIMER_STATUS_FAIL)
			return INTERVAL_TIMER_STATUS_FAIL;
	}
	return INTERVAL_TIMER_STATUS_OK;
}

// Reset all 3 timers.
uint32_t intervalTimer_resetAll()
{
	for(uint8_t timerNumber = 0; timerNumber < INTERVAL_TIMER_NUMBER_OF_TIMERS; timerNumber++)
	{
		if(intervalTimer_reset(timerNumber) == INTERVAL_TIMER_STATUS_FAIL)
			return INTERVAL_TIMER_STATUS_FAIL;
	}
	return INTERVAL_TIMER_STATUS_OK;
}

// Test all 3 timers.
uint32_t intervalTimer_testAll()
{
	for(uint8_t timerNumber = 0; timerNumber < INTERVAL_TIMER_NUMBER_OF_TIMERS; timerNumber++)
	{
		if(intervalTimer_runTest(timerNumber) == INTERVAL_TIMER_STATUS_FAIL)
			return INTERVAL_TIMER_STATUS_FAIL;
	}
	return INTERVAL_TIMER_STATUS_OK;
}

// Test a timer.
uint32_t intervalTimer_runTest(uint32_t timerNumber)
{
	intervalTimer_init(timerNumber);
	intervalTimer_reset(timerNumber);

	//Ensure that timer is actually reset.
	if(intervalTimer_getTotalDurationInTicks(timerNumber) != 0)
	{
		printf("Timer %d reset, but value is not zero.\r\n", (int)timerNumber);
		return INTERVAL_TIMER_STATUS_FAIL;
	}

	//Make sure timer is not running
	utils_msDelay(INTERVAL_TIMER_SHORT_MS_DELAY);
	if(intervalTimer_getTotalDurationInTicks(timerNumber) != 0)
	{
		printf("Timer %d reset, but value is changing.\r\n", (int)timerNumber);
		return INTERVAL_TIMER_STATUS_FAIL;
	}

	//Start timer and ensure it has started
	intervalTimer_start(timerNumber);
	utils_msDelay(INTERVAL_TIMER_SHORT_MS_DELAY);
	uint64_t value1 = intervalTimer_getTotalDurationInTicks(timerNumber);
	if(value1 == 0)
	{
		printf("Timer %d started, but value is not changing.\r\n", (int)timerNumber);
		return INTERVAL_TIMER_STATUS_FAIL;
	}

	//Make sure value is still changing
	utils_msDelay(INTERVAL_TIMER_SHORT_MS_DELAY);
	uint64_t value2 = intervalTimer_getTotalDurationInTicks(timerNumber);
	if(value2 <= value1)
	{
		printf("Timer %d value is not going up.\r\n", (int)timerNumber);
		return INTERVAL_TIMER_STATUS_FAIL;
	}

	//Make sure timer stops
	intervalTimer_stop(timerNumber);
	value1 = intervalTimer_getTotalDurationInTicks(timerNumber);
	utils_msDelay(INTERVAL_TIMER_SHORT_MS_DELAY);
	value2 = intervalTimer_getTotalDurationInTicks(timerNumber);
	if(value1 != value2)
	{
		printf("Timer %d stopped, but value is still changing.\r\n", (int)timerNumber);
		return INTERVAL_TIMER_STATUS_FAIL;
	}

	//Passed all tests
	return INTERVAL_TIMER_STATUS_OK;
}

// Get the total duration in seconds of timer.
uint64_t intervalTimer_getTotalDurationInTicks(uint32_t timerNumber)
{
	if(intervalTimer_CheckValidTimerAndInitialized(timerNumber) == INTERVAL_TIMER_STATUS_FAIL)
		return INTERVAL_TIMER_STATUS_FAIL;
	uint64_t upperValue = intervalTimer_readTimerRegister(timerNumber, INTERVAL_TIMER_TCR1_OFFSET);
	uint64_t lowerValue = intervalTimer_readTimerRegister(timerNumber, INTERVAL_TIMER_TCR0_OFFSET);

	//Make sure upper value hasn't changed. (if a rollover occurred, we would have a large error!)
	uint64_t newUpperValue = intervalTimer_readTimerRegister(timerNumber, INTERVAL_TIMER_TCR1_OFFSET);
	if(newUpperValue != upperValue)
	{
		upperValue = newUpperValue;
		lowerValue = intervalTimer_readTimerRegister(timerNumber, INTERVAL_TIMER_TCR0_OFFSET);
	}
	return (upperValue << INTERVAL_TIMER_TCR_WIDTH) | lowerValue;
}

// Get the total duration in seconds of timer.
uint32_t intervalTimer_getDurationInTicks(uint32_t timerNumber)
{
	if(intervalTimer_CheckValidTimerAndInitialized(timerNumber) == INTERVAL_TIMER_STATUS_FAIL)
		return INTERVAL_TIMER_STATUS_FAIL;
	uint32_t lowerValue = intervalTimer_readTimerRegister(timerNumber, INTERVAL_TIMER_TCR0_OFFSET);
	return lowerValue;
}

uint32_t intervalTimer_getTotalDurationInSeconds(uint32_t timerNumber, double *seconds)
{
	if(intervalTimer_CheckValidTimerAndInitialized(timerNumber) == INTERVAL_TIMER_STATUS_FAIL)
		return INTERVAL_TIMER_STATUS_FAIL;
	double timerFreq = intervalTimer_getTimerFrequency(timerNumber);
	double ticks = (double)intervalTimer_getTotalDurationInTicks(timerNumber);
	*seconds = ticks / timerFreq;
	return (uint32_t)seconds;
}

// Helper function to read GPIO registers.
int32_t intervalTimer_readTimerRegister(uint32_t timerNumber, int32_t offset)
{
	return Xil_In32(intervalTimer_getTimerBaseAddress(timerNumber) + offset);
}

// Helper function to write GPIO registers.
void intervalTimer_writeTimerRegister(uint32_t timerNumber, int32_t offset, int32_t value)
{
	Xil_Out32(intervalTimer_getTimerBaseAddress(timerNumber) + offset, value);
}

// Helper function to write GPIO registers.
void intervalTimer_writeTimerRegisterBit(uint32_t timerNumber, int32_t offset, int32_t k, int32_t b)
{
	uint32_t regValue = (uint32_t)intervalTimer_readTimerRegister(timerNumber, offset);
	regValue = intervalTimer_setBit(regValue, k, b);
	intervalTimer_writeTimerRegister(timerNumber, offset, regValue);
}

// Helper function to write GPIO registers.
int32_t intervalTimer_readTimerRegisterBit(uint32_t timerNumber, int32_t offset, int32_t k, int32_t b)
{
	uint32_t regValue = (uint32_t)intervalTimer_readTimerRegister(timerNumber, offset);
	return intervalTimer_getBit((u32)regValue, k);
}

// Sets the kth bit
int32_t intervalTimer_setBit(int32_t x, int32_t k, int32_t b)
{
   return (b ? (x | (INTERVAL_TIMER_SINGLE_BIT << k))  :  (x & ~(INTERVAL_TIMER_SINGLE_BIT << k)));
}

// Gets the kth bit
int32_t intervalTimer_getBit(int32_t x, int32_t k)
{
   return ((x & (INTERVAL_TIMER_SINGLE_BIT << k)) != 0);
}

int32_t intervalTimer_getTimerFrequency(uint32_t timerNumber)
{
	switch(timerNumber)
	{
	case INTERVAL_TIMER_0_ID:
		return INTERVAL_TIMER_0_CLOCK_FREQ_HZ;
		break;
	case INTERVAL_TIMER_1_ID:
		return INTERVAL_TIMER_1_CLOCK_FREQ_HZ;
		break;
	case INTERVAL_TIMER_2_ID:
		return INTERVAL_TIMER_2_CLOCK_FREQ_HZ;
		break;
	default:
		return INTERVAL_TIMER_STATUS_FAIL;
		break;
	}
}

int32_t intervalTimer_getTimerBaseAddress(uint32_t timerNumber)
{
	switch(timerNumber)
	{
	case INTERVAL_TIMER_0_ID:
		return INTERVAL_TIMER_0_BASEADDR;
		break;
	case INTERVAL_TIMER_1_ID:
		return INTERVAL_TIMER_1_BASEADDR;
		break;
	case INTERVAL_TIMER_2_ID:
		return INTERVAL_TIMER_2_BASEADDR;
		break;
	default:
		return INTERVAL_TIMER_STATUS_FAIL;
		break;
	}
}

uint8_t intervalTimer_CheckValidTimerAndInitialized(uint32_t timerNumber)
{
	if(intervalTimer_initializeFlag[timerNumber] == INTERVAL_TIMER_STATUS_FAIL)
	{
		printf("Error. Timer %d is not yet initialized\r\n", (int)timerNumber);
		return INTERVAL_TIMER_STATUS_FAIL;
	}

	if(timerNumber < 0 || timerNumber >= INTERVAL_TIMER_NUMBER_OF_TIMERS)
	{
		printf("Error. Timer %d does not exist. There are only %d timers.\r\n", (int)timerNumber, INTERVAL_TIMER_NUMBER_OF_TIMERS);
		return INTERVAL_TIMER_STATUS_FAIL;
	}
	return INTERVAL_TIMER_STATUS_OK;
}
