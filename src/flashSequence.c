#include "flashSequence.h"
#include "simonDisplay.h"
#include "supportFiles/display.h"
#include "globals.h"
#include "supportFiles/utils.h"
#include "stdio.h"


// This will set the sequence to a simple sequential pattern.
// It starts by flashing the first color, and then increments the index and flashes the first
// two colors and so forth. Along the way it prints info messages to the LCD screen.
#define TEST_SEQUENCE_LENGTH 8	// Just use a short test sequence.
uint8_t flashSequence_testSequence[TEST_SEQUENCE_LENGTH] = {SIMON_DISPLAY_REGION_0,
							    SIMON_DISPLAY_REGION_1,
							    SIMON_DISPLAY_REGION_2,
							    SIMON_DISPLAY_REGION_3,
							    SIMON_DISPLAY_REGION_3,
							    SIMON_DISPLAY_REGION_2,
							    SIMON_DISPLAY_REGION_1,
							    SIMON_DISPLAY_REGION_0};
#define INCREMENTING_SEQUENCE_MESSAGE1 "Incrementing Sequence"	// Info message.
#define RUN_TEST_COMPLETE_MESSAGE "Runtest() Complete"		// Info message.
#define MESSAGE_TEXT_SIZE 2	                                // Make the text easy to see.

#define DELAY_DURATION (.5 / GLOBALS_TIMER_PERIOD) // .5 s

enum flashSequence_states
	{initial_st,
	flashRegion_st,
	delay_timer_st,
	final_st} flashSequence_state;

bool completed;
uint16_t index;
bool flashSequence_enabled;

// Turn on the state machine. Part of the interlock.
void flashSequence_enable()
{
	flashSequence_enabled = true;
}

// Turn off the state machine. Part of the interlock.
void flashSequence_disable()
{
	flashSequence_enabled = false;
}

bool flashSequence_completed()
{
	return completed;
}


// Standard tick function.
void flashSequence_tick()
{
	static int32_t delayTimer;

	// Current state actions
	switch (flashSequence_state)
	{
	case initial_st:
	break;
	case flashRegion_st:
	break;
	case delay_timer_st:
		delayTimer++;
	break;
	case final_st:
	break;
	}

	// State transition
	switch (flashSequence_state)
	{
	case initial_st:
		if(flashSequence_enabled)
		{
			flashSequence_state = flashRegion_st;
			completed = false;
			index = 0;
		}
	break;
	case flashRegion_st:
		//Flash next region in sequence
		simonDisplay_drawSquare(globals_getSequenceValue(index), false);
		delayTimer = 0;
		flashSequence_state = delay_timer_st;
	break;
	case delay_timer_st:
		if(delayTimer == DELAY_DURATION)
		{
			// Blank last region in sequence
			simonDisplay_drawSquare(globals_getSequenceValue(index), true);
			index++;
			if(index < globals_getSequenceIterationLength())
			{
				flashSequence_state = flashRegion_st;
			}
			else
			{
				completed = true;
				flashSequence_state = final_st;
			}
		}
	break;
	case final_st:
		if(!flashSequence_enabled)
		{
			flashSequence_state = initial_st;
		}
	break;
	}
}

// Print the incrementing sequence message.
void flashSequence_printIncrementingMessage() {
  display_fillScreen(DISPLAY_BLACK);// Otherwise, tell the user that you are incrementing the sequence.
  display_setCursor(0, display_height()/2);	    // Roughly centered.
  display_println(INCREMENTING_SEQUENCE_MESSAGE1);  // Print the message.
  utils_msDelay(2000);                              // Hold on for 2 seconds.
  display_fillScreen(DISPLAY_BLACK);		    // Clear the screen.
}

void flashSequence_runTest() {
  display_init();	// We are using the display.
  display_fillScreen(DISPLAY_BLACK);	// Clear the display.
  globals_setSequence(flashSequence_testSequence, TEST_SEQUENCE_LENGTH);	// Set the sequence.
  flashSequence_enable();			        // Enable the flashSequence state machine.
  int16_t sequenceLength = 1;	                        // Start out with a sequence of length 1.
  globals_setSequenceIterationLength(sequenceLength);	// Set the iteration length.
  display_setTextSize(MESSAGE_TEXT_SIZE);	        // Use a standard text size.
  while (1) {	                // Run forever unless you break.
    flashSequence_tick();	// tick the state machine.
    utils_msDelay(1);	// Provide a 1 ms delay.
    if (flashSequence_completed()) {  // When you are done flashing the sequence.
      flashSequence_disable();  // Interlock by first disabling the state machine.
      flashSequence_tick();	// tick is necessary to advance the state.
      utils_msDelay(1);		// don't really need this here, just for completeness.
      flashSequence_enable();	// Finish the interlock by enabling the state machine.
      utils_msDelay(1);	// Wait 1 ms for no good reason.
      sequenceLength++;	// Increment the length of the sequence.
      if (sequenceLength > TEST_SEQUENCE_LENGTH) // Stop if you have done the full sequence.
        break;
      flashSequence_printIncrementingMessage();  // Tell the user that you are going to the next step in the pattern.
      globals_setSequenceIterationLength(sequenceLength);	// Set the length of the pattern.
    }
  }
  // Let the user know that you are completed.
  display_fillScreen(DISPLAY_BLACK);
  display_setCursor(0, display_height()/2);
  display_println(RUN_TEST_COMPLETE_MESSAGE);
}
