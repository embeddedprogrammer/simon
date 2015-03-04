#include "buttons.h"
#include "supportFiles/display.h"
#include "xparameters.h"
#include <stdint.h>
#include <stdio.h>

//Internal functions
int32_t buttons_readGpioRegister(int32_t offset);
void buttons_writeGpioRegister(int32_t offset, int32_t value);
void buttons_drawButtonVisual(int32_t position, char value);
int32_t buttons_setBit(int32_t x, int32_t k, int32_t b);
int32_t buttons_getBit(int32_t x, int32_t k);

#define BUTTONS_GPIO_DEVICE_BASE_ADDRESS XPAR_GPIO_PUSH_BUTTONS_BASEADDR
#define TRI_STATE_OFFSET 4
#define FOUR_BIT_TRI_STATE_INPUT 0xF

// Initialize buttons
int buttons_init()
{
	// Make sure tri-state driver is set to off so we can read from register.
	buttons_writeGpioRegister(TRI_STATE_OFFSET, FOUR_BIT_TRI_STATE_INPUT);
	return BUTTONS_INIT_STATUS_OK;
}

// Helper function to read GPIO registers.
int32_t buttons_readGpioRegister(int32_t offset)
{
  // Note that you have to include a cast (uint32_t *) to keep the compiler happy.
  uint32_t *ptr = (uint32_t *) BUTTONS_GPIO_DEVICE_BASE_ADDRESS + offset;
  return *ptr;
}

// Helper function to write GPIO registers.
void buttons_writeGpioRegister(int32_t offset, int32_t value)
{
  // Note that you have to include a cast (uint32_t *) to keep the compiler happy.
  uint32_t *ptr = (uint32_t *) BUTTONS_GPIO_DEVICE_BASE_ADDRESS + offset;
  *ptr = value;
}

// Returns the current value of all 4 buttons as the lower 4 bits of the returned value.
// bit3 = BTN3, bit2 = BTN2, bit1 = BTN1, bit0 = BTN0.
int32_t buttons_read()
{
	return buttons_readGpioRegister(0x0);
}

int32_t buttons_displayWidth;
int32_t buttons_displayHeight;

// Runs a test of the buttons. As you push the buttons, graphics and messages will be written to the LCD
// panel. The test will until all 4 pushbuttons are simultaneously pressed.
void buttons_runTest()
{
	buttons_init();

	display_init();  // Must init all of the software and underlying hardware for LCD.
	display_fillScreen(DISPLAY_BLACK);  // Blank the screen.

	buttons_displayWidth = display_width();
	buttons_displayHeight = display_height();

	// Cache value of buttons to reduce number of accesses to buttons_read.
	// Too many frequent reads can also cause problems with the hardware.
	// Also keep an old value to compare with to reduce screen flashing.
	int32_t lastButtonsValue = 0x0;
	int32_t newButtonsValue = 0x0;

	while(newButtonsValue != 0xf)
	{
		newButtonsValue = buttons_read();
		if(lastButtonsValue != newButtonsValue)
		{
			for(char i = 0; i < 4; i++)
			{
				if(buttons_getBit(lastButtonsValue, i) != buttons_getBit(newButtonsValue, i))
				{
					buttons_drawButtonVisual(i, buttons_getBit(newButtonsValue, i));
				}
			}
			lastButtonsValue = newButtonsValue;
		}
	}

	// Clear screen when done
	display_fillScreen(DISPLAY_BLACK);
}

#define TEXT_HORIZONTAL_MARGIN 15
#define TEXT_VERTICAL_OFFSET 10

// Draw a rectangle on the screen to visualize a button press.
void buttons_drawButtonVisual(int32_t position, char value)
{
	// Determine color of rectangle to draw
	int32_t rectangleColor = DISPLAY_BLACK;
	switch ((position + 1) * value)
	{
	// If erasing rectangle, set to black
	case 0:
		rectangleColor = DISPLAY_BLACK;
		break;

	// Otherwise set to appropriate color, depending on the position.
	case 1:
		rectangleColor = DISPLAY_YELLOW;
		break;
	case 2:
		rectangleColor = DISPLAY_CYAN;
		break;
	case 3:
		rectangleColor = DISPLAY_RED;
		break;
	case 4:
		rectangleColor = DISPLAY_BLUE;
		break;
	}
	int32_t x = buttons_displayWidth / 4 * (3 - position);

	// Draw rectangle
	display_fillRect(x, 0, buttons_displayWidth / 4, buttons_displayHeight / 2, rectangleColor);

	// If needed, add text
	if(value)
	{
		display_setCursor(x + TEXT_HORIZONTAL_MARGIN, buttons_displayHeight / 4 - TEXT_VERTICAL_OFFSET);
		display_setTextColor((position >= 2) ? DISPLAY_WHITE : DISPLAY_BLACK);
		char str[5] = "BTNx";
		str[3] = '0' + position;
		display_setTextSize(2);
		display_println(str);
	}
}

// Sets the kth bit
int32_t buttons_setBit(int32_t x, int32_t k, int32_t b)
{
   return (b ?  (x | (0x01 << k))  :  (x & ~(0x01 << k)));
}

// Gets the kth bit
int32_t buttons_getBit(int32_t x, int32_t k)
{
   return ((x & (0x01 << k)) != 0);
}
