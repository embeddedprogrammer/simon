#include "supportFiles/display.h"
#include "supportFiles/utils.h"
#include "stdio.h"
#include "simonDisplay.h"

#define TOUCH_PANEL_ANALOG_PROCESSING_DELAY_IN_MS 60 // in ms
#define MAX_STR 255
#define TEXT_SIZE 2
// Runs a brief demonstration of how buttons can be pressed and squares lit up to implement the user
// interface of the Simon game. The routine will continue to run until the touchCount has been reached, e.g.,
// the user has touched the pad touchCount times.

int8_t simonDisplay_computeRegionNumber(int16_t x, int16_t y)
{
	return (y > display_height() / 2) * 2 + (x > display_width() / 2);
}

uint16_t getRegionColor(uint8_t regionNo)
{
	switch(regionNo)
	{
	case SIMON_DISPLAY_REGION_0: return DISPLAY_RED;
	case SIMON_DISPLAY_REGION_1: return DISPLAY_YELLOW;
	case SIMON_DISPLAY_REGION_2: return DISPLAY_BLUE;
	case SIMON_DISPLAY_REGION_3: return DISPLAY_GREEN;
	}
}

// Draws a colored "button" that the user can touch.
// The colored button is centered in the region but does not fill the region.
void simonDisplay_drawButton(uint8_t regionNumber)
{
	int16_t x = (regionNumber % 2) ? display_width() * 5 / 8 : display_width() / 8;
	int16_t y = (regionNumber / 2) ? display_height() * 5 / 8 : display_width() / 8;
	display_fillRect(x, y, display_width() / 4, display_height() / 4, getRegionColor(regionNumber));
}

// Erases button
void simonDisplay_eraseButton(uint8_t regionNumber)
{
	int16_t x = (regionNumber % 2) ? display_width() * 5 / 8 : display_width() / 8;
	int16_t y = (regionNumber / 2) ? display_height() * 5 / 8 : display_width() / 8;
	display_fillRect(x, y, display_width() / 4, display_height() / 4, DISPLAY_BLACK);
}

// Convenience function that draws all of the buttons.
void simonDisplay_drawAllButtons()
{
	for(int i = 0; i < SIMON_DISPLAY_NUMBER_OF_REGIONS; i++)
		simonDisplay_drawButton(i);
}

// Convenience function that draws all of the buttons.
void simonDisplay_eraseAllButtons()
{
	for(int i = 0; i < SIMON_DISPLAY_NUMBER_OF_REGIONS; i++)
		simonDisplay_eraseButton(i);
}

// Draws a bigger square that completely fills the region.
// If the erase argument is true, it draws the square as black background to "erase" it.
void simonDisplay_drawSquare(uint8_t regionNo, bool erase)
{
	int16_t x = (regionNo % 2) ? display_width() / 2 : 0;
	int16_t y = (regionNo / 2) ? display_height() / 2 : 0;
	uint16_t color = erase ? DISPLAY_BLACK : getRegionColor(regionNo);
	display_fillRect(x, y, display_width() / 2, display_height() / 2, color);
}

// I used a busy-wait delay (utils_msDelay) that uses a for-loop and just blocks until the time has passed.
// When you implement the game, you CANNOT use this function as we discussed in class. Implement the delay
// using the non-blocking state-machine approach discussed in class.
void simonDisplay_runTest(uint16_t touchCount)
{
  display_init();  // Always initialize the display.
  char str[MAX_STR];   // Enough for some simple printing.
  uint8_t regionNumber;
  uint16_t touches = 0;
  // Write an informational message and wait for the user to touch the LCD.
  display_fillScreen(DISPLAY_BLACK);        // clear the screen.
  display_setCursor(0, display_height()/2); //
  display_setTextSize(TEXT_SIZE);
  display_setTextColor(DISPLAY_RED, DISPLAY_BLACK);
  sprintf(str, "Touch and release to start the Simon demo.");
  display_println(str);
  display_println();
  sprintf(str, "Demo will terminate after %d touches.", touchCount);
  display_println(str);
  while (!display_isTouched());       // Wait here until the screen is touched.
  while (display_isTouched());        // Now wait until the touch is released.
  display_fillScreen(DISPLAY_BLACK);  // Clear the screen.
  simonDisplay_drawAllButtons();      // Draw all of the buttons.
  bool touched = false;  	      // Keep track of when the pad is touched.
  int16_t x, y;  		      // Use these to keep track of coordinates.
  uint8_t z;      		      // This is the relative touch pressure.
  while (touches < touchCount) {  // Run the loop according to the number of touches passed in.
    if (!display_isTouched() && touched) {         // user has stopped touching the pad.
      simonDisplay_drawSquare(regionNumber, true); // Erase the square.
      simonDisplay_drawButton(regionNumber);	   // DISPLAY_REDraw the button.
      touched = false;															// Released the touch, set touched to false.
    } else if (display_isTouched() && !touched) {   // User started touching the pad.
      touched = true;                               // Just touched the pad, set touched = true.
      touches++;  																	// Keep track of the number of touches.
      display_clearOldTouchData();  // Get rid of data from previous touches.
      // Must wait this many milliseconds for the chip to do analog processing.
      utils_msDelay(TOUCH_PANEL_ANALOG_PROCESSING_DELAY_IN_MS);
      display_getTouchedPoint(&x, &y, &z);                  // After the wait, get the touched point.
      regionNumber = simonDisplay_computeRegionNumber(x, y);// Compute the region number.
      simonDisplay_drawSquare(regionNumber, false);	    // Draw the square (erase = false).
    }
  }
  // Done with the demo, write an informational message to the user.
  display_fillScreen(DISPLAY_BLACK);        // clear the screen.
  display_setCursor(0, display_height()/2); // Place the cursor in the middle of the screen.
  display_setTextSize(2);                   // Make it readable.
  display_setTextColor(DISPLAY_RED, DISPLAY_BLACK);  // red is foreground color, black is background color.
  sprintf(str, "Simon demo terminated");    // Format a string using sprintf.
  display_println(str);                     // Print it to the LCD.
  sprintf(str, "after %d touches.", touchCount);  // Format the rest of the string.
  display_println(str);  // Print it to the LCD.
}
