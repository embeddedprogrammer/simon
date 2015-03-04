#define INTERVAL_TIMER_STATUS_OK 1
#define INTERVAL_TIMER_STATUS_FAIL 0

#include <stdint.h>

#ifndef INTERVAL_H_
#define INTERVAL_H_

#define INTERVAL_TIMER_NUMBER_OF_TIMERS 3

// Start timer. Init must be called beforehand or timer will display an error.
uint32_t intervalTimer_start(uint32_t timerNumber);
// Pause timer.
uint32_t intervalTimer_stop(uint32_t timerNumber);
// Load a zero into timer. Does not stop timer if running.
uint32_t intervalTimer_reset(uint32_t timerNumber);
// Initialize timer. Load a zero into timer and set to count up.
uint32_t intervalTimer_init(uint32_t timerNumber);
// Initialize all 3 timers.
uint32_t intervalTimer_initAll();
// Reset all 3 timers.
uint32_t intervalTimer_resetAll();
// Test all 3 timers.
uint32_t intervalTimer_testAll();
// Test a timer.
uint32_t intervalTimer_runTest(uint32_t timerNumber);
// Get the total duration in seconds of timer. Valid values from 0 seconds to the equivalent of 300 years.
uint32_t intervalTimer_getTotalDurationInSeconds(uint32_t timerNumber, double *seconds);
// Get the total duration in ticks of timer
uint32_t intervalTimer_getDurationInTicks(uint32_t timerNumber);

#endif /* INTERVAL_H_ */
