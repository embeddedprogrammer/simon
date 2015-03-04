#define RUN_TEST_TERMINATION_MESSAGE1 "buttonHandler_runTest()"
#define RUN_TEST_TERMINATION_MESSAGE2 "terminated."
#define RUN_TEST_TEXT_SIZE 2
#define AD_WAIT_DURATION  (100E-3 / GLOBALS_TIMER_PERIOD) //  100 ms

#include "globals.h"
#include "buttonHandler.h"
#include "supportFiles/display.h"
#include "supportFiles/utils.h"
#include "simonDisplay.h"
#include "stdio.h"

enum buttonHandler_states
	{initial_st,
	wait_for_touch_st,
	ad_timer_st,
	wait_for_release_st,
	final_st} buttonHandler_state;

bool pressed;
uint8_t regionPressed;

// Get the simon region numbers. See the source code for the region numbering scheme.
void buttonHandler_calculateRegion()
{
	// get touched coords
	int16_t touched_x;
	int16_t touched_y;
	uint8_t touched_z;
	display_getTouchedPoint(&touched_x, &touched_y, &touched_z);
	regionPressed = simonDisplay_computeRegionNumber(touched_x, touched_y);
}

bool enabled;

// Turn on the state machine. Part of the interlock.
void buttonHandler_enable()
{
	enabled = true;
}

// Turn off the state machine. Part of the interlock.
void buttonHandler_disable()
{
	enabled = false;
}

// Get the simon region numbers. See the source code for the region numbering scheme.
uint8_t buttonHandler_getRegionNumber()
{
	return regionPressed;
}

// Other state machines can call this function to see if the user has stopped touching the pad.
bool buttonHandler_releaseDetected()
{
	return pressed;
}

// Standard tick function.
void buttonHandler_tick()
{
	static int32_t adTimer;

	// Current state actions
	switch (buttonHandler_state)
	{
	case initial_st:
	break;
	case wait_for_touch_st:
	break;
	case ad_timer_st:
		adTimer++;
	break;
	case wait_for_release_st:
	break;
	case final_st:
	break;
	}

	// State transition
	switch (buttonHandler_state)
	{
	case initial_st:
		if(enabled)
		{
			buttonHandler_state = wait_for_touch_st;
			pressed = false;
		}
	break;
	case wait_for_touch_st:
		if(display_isTouched())
		{
			buttonHandler_state = ad_timer_st;
			adTimer = 0;
			display_clearOldTouchData();
			buttonHandler_state = ad_timer_st;
		}
	break;
	case ad_timer_st:
		if(adTimer == AD_WAIT_DURATION)
		{
			buttonHandler_calculateRegion();
			simonDisplay_drawSquare(regionPressed, false);
			buttonHandler_state = wait_for_release_st;
		}
	break;
	case wait_for_release_st:
		if(!display_isTouched())
		{
			simonDisplay_drawSquare(regionPressed, true);
			simonDisplay_drawButton(regionPressed);
			pressed = true;
			buttonHandler_state = final_st;
		}
	break;
	case final_st:
		if(!enabled)
		{
			buttonHandler_state = initial_st;
		}
	break;
	}
}

// buttonHandler_runTest(int16_t touchCount) runs the test until
// the user has touched the screen touchCount times. It indicates
// that a button was pushed by drawing a large square while
// the button is pressed and then erasing the large square and
// redrawing the button when the user releases their touch.
void buttonHandler_runTest(int16_t touchCountArg)
{
  int16_t touchCount = 0;             // Keep track of the number of touches.
  display_init();                     // Always have to init the display.
  display_fillScreen(DISPLAY_BLACK);  // Clear the display.
  simonDisplay_drawAllButtons();      // Draw the four buttons.
  buttonHandler_enable();
  while (touchCount < touchCountArg) {  // Loop here while touchCount is less than the touchCountArg
    buttonHandler_tick();               // Advance the state machine.
    utils_msDelay(1);			// Wait here for 1 ms.
    if (buttonHandler_releaseDetected()) {  // If a release is detected, then the screen was touched.
      touchCount++;                         // Keep track of the number of touches.
      printf("button released: %d\n\r", buttonHandler_getRegionNumber());  // Get the region number that was touched.
      buttonHandler_disable();             // Interlocked behavior: handshake with the button handler (now disabled).
      utils_msDelay(1);                     // wait 1 ms.
      buttonHandler_tick();                 // Advance the state machine.
      buttonHandler_enable();               // Interlocked behavior: enable the buttonHandler.
      utils_msDelay(1);                     // wait 1 ms.
      buttonHandler_tick();                 // Advance the state machine.
    }
  }
  display_fillScreen(DISPLAY_BLACK);			// clear the screen.
  display_setTextSize(RUN_TEST_TEXT_SIZE);		// Set the text size.
  display_setCursor(0, display_height()/2);		// Move the cursor to a rough center point.
  display_println(RUN_TEST_TERMINATION_MESSAGE1);	// Print the termination message on two lines.
  display_println(RUN_TEST_TERMINATION_MESSAGE2);
}
