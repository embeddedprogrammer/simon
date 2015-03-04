#include "simonControl.h"
#include "stdlib.h"
#include "supportFiles/display.h"
#include "stdio.h"
#include "intervalTimer.h"
#include "flashSequence.h"
#include "verifySequence.h"
#include "globals.h"
#include "string.h"
#include "simonDisplay.h"
#include "supportFiles/utils.h"
#include "buttonHandler.h"

#define CONGRATS_TIMER_DURATION                 (1 / GLOBALS_TIMER_PERIOD)       // 1s
#define NEW_LEVEL_TIMOUT_TIMER_DURATION         (5 / GLOBALS_TIMER_PERIOD)       // 5s
#define DISPLAY_SCORE_TIMER_DURATION            (4 / GLOBALS_TIMER_PERIOD)       // 4s
#define PAUSE_BEFORE_FLASHING_SEQUENCE_DURATION (500E-3 / GLOBALS_TIMER_PERIOD)  // 500ms

#define STARTING_LEVEL_SEQUENCE_LENGTH 4
#define NEW_LEVEL_INCREMENT_SEQUENCE_AMOUNT 1

#define TEXT_HEIGHT 8
#define TEXT_WIDTH 6
#define TEXT_SIZE_BIG 6
#define TEXT_SIZE_MED 4
#define TEXT_SIZE_SMALL 2

#define MESSAGE_NONE 0
#define MESSAGE_INTRO 1
#define MESSAGE_CONGRATULATIONS 2
#define MESSAGE_TOUCH_NEW_LEVEL 3
#define MESSAGE_DISPLAY_SCORE 4

uint8_t currentlyDisplayedMessage;

enum simonControl_states
	{initial_st,
	touch_to_start_st,
	wait_for_release_st,
	pause_before_flash_st,
	flash_sequence_st,
	verify_sequence_st,
	congrats_st,
	touch_for_new_level_st,
	display_score_st} simonControl_state;

uint16_t longestSuccessfulSequence;

int32_t congratsTimer;
int32_t newLevelTimoutTimer;
int32_t displayScoreTimer;
int32_t pauseTimer;

//Internal functions
void prepAndEnterState(simonControl_states newState);

void generateRandomSequence(uint16_t sequenceLength)
{
	uint8_t sequence[GLOBALS_MAX_FLASH_SEQUENCE];
	intervalTimer_stop(0);
	srand(intervalTimer_getDurationInTicks(0));
	for(int i = 0; i < sequenceLength; i++)
	{
		sequence[i] = rand() / (RAND_MAX / SIMON_DISPLAY_NUMBER_OF_REGIONS);
	}
	globals_setSequence(sequence, sequenceLength);
}

// Centers text at a certain height. Returns the vertical position of the next line.
int32_t displayText(char * str, int32_t textSize, int32_t height, uint16_t color)
{
	display_setCursor((display_width() - TEXT_WIDTH * textSize * strlen(str)) / 2, height);
	display_setTextColor(color);
	display_setTextSize(textSize);
	display_println(str);
	return height + TEXT_HEIGHT * textSize;
}

// Centers text.
void displayTextCentered(char * str, int32_t textSize, uint16_t color)
{
	displayText(str, textSize, (display_height() - TEXT_HEIGHT * textSize) / 2, color);
}

void showIntroScreen(bool erase)
{
	int32_t verticalPosition = (display_height() - TEXT_HEIGHT * (TEXT_SIZE_BIG + TEXT_SIZE_SMALL)) / 2;
	verticalPosition = displayText("Simon", TEXT_SIZE_BIG, verticalPosition, erase ? DISPLAY_BLACK : DISPLAY_WHITE);
	displayText("Touch to start", TEXT_SIZE_SMALL, verticalPosition, erase ? DISPLAY_BLACK : DISPLAY_WHITE);
	currentlyDisplayedMessage = erase ? MESSAGE_NONE : MESSAGE_INTRO;
}

void congratulateUser(bool erase)
{
	displayTextCentered("Yay!", TEXT_SIZE_MED, erase ? DISPLAY_BLACK : DISPLAY_WHITE);
	currentlyDisplayedMessage = erase ? MESSAGE_NONE : MESSAGE_CONGRATULATIONS;
}

void touchForNewLevel(bool erase)
{
	displayTextCentered("Touch for new level", TEXT_SIZE_SMALL, erase ? DISPLAY_BLACK : DISPLAY_WHITE);
	currentlyDisplayedMessage = erase ? MESSAGE_NONE : MESSAGE_TOUCH_NEW_LEVEL;
}

void displayScore(bool erase)
{
	char buffer[25];
	sprintf(buffer, "Longest Sequence: %d", longestSuccessfulSequence);
	displayTextCentered(buffer, TEXT_SIZE_SMALL, erase ? DISPLAY_BLACK : DISPLAY_WHITE);
	currentlyDisplayedMessage = erase ? MESSAGE_NONE : MESSAGE_DISPLAY_SCORE;
}

void eraseMessage()
{
	switch(currentlyDisplayedMessage)
	{
	case MESSAGE_INTRO:
		showIntroScreen(true);
	case MESSAGE_CONGRATULATIONS:
		congratulateUser(true);
	case MESSAGE_TOUCH_NEW_LEVEL:
		touchForNewLevel(true);
	case MESSAGE_DISPLAY_SCORE:
		displayScore(true);
	}
}

// Standard tick function.
void simonControl_tick()
{
	// Current state actions
	switch (simonControl_state)
	{
	case initial_st:
		break;
	case touch_to_start_st:
		break;
	case wait_for_release_st:
		break;
	case pause_before_flash_st:
		pauseTimer++;
		break;
	case flash_sequence_st:
		break;
	case verify_sequence_st:
		break;
	case congrats_st:
		congratsTimer++;
		break;
	case touch_for_new_level_st:
		newLevelTimoutTimer++;
		break;
	case display_score_st:
		displayScoreTimer++;
		break;
	}

	// State transition
	switch (simonControl_state)
	{
	case initial_st:
		intervalTimer_init(0);
		prepAndEnterState(touch_to_start_st);
		break;
	case touch_to_start_st:
		if(display_isTouched())
		{
			printf("Touched\r\n");
			//Init beginning level
			longestSuccessfulSequence = 0;
			generateRandomSequence(STARTING_LEVEL_SEQUENCE_LENGTH);
			globals_setSequenceIterationLength(0);
			prepAndEnterState(wait_for_release_st);
		}
		break;
	case wait_for_release_st:
		if(!display_isTouched())
		{
			printf("Released - Blank screen\r\n");
			prepAndEnterState(pause_before_flash_st);
		}
		break;
	case pause_before_flash_st:
		if(pauseTimer > PAUSE_BEFORE_FLASHING_SEQUENCE_DURATION)
		{
			printf("Flashing sequence\r\n");
			prepAndEnterState(flash_sequence_st);
		}
		break;
	case flash_sequence_st:
		if(flashSequence_completed())
		{
			printf("Verifying sequence\r\n");
			printf("   Blank screen\r\n");
			prepAndEnterState(verify_sequence_st);
		}
		break;
	case verify_sequence_st:
		if(verifySequence_isComplete())
		{
			verifySequence_disable();
			// User error or timeout error?
			if(verifySequence_isTimeOutError() || verifySequence_isUserInputError())
			{
				if(verifySequence_isTimeOutError())
					printf("Timeout error\r\n");
				if(verifySequence_isUserInputError())
					printf("User error\r\n");
				prepAndEnterState(display_score_st);
			}
			// End of sequence?
			else if(globals_getSequenceIterationLength() == globals_getSequenceLength())
				prepAndEnterState(congrats_st);
			// If not end, go to next iteration
			else
			{
				prepAndEnterState(wait_for_release_st);
			}
		}
		break;
	case congrats_st:
		if(congratsTimer == CONGRATS_TIMER_DURATION)
		{
			prepAndEnterState(touch_for_new_level_st);
		}
		break;
	case touch_for_new_level_st:
		if(display_isTouched())
		{
			generateRandomSequence(globals_getSequenceLength() + NEW_LEVEL_INCREMENT_SEQUENCE_AMOUNT);
			globals_setSequenceIterationLength(0);
			prepAndEnterState(wait_for_release_st);
		}
		else if(newLevelTimoutTimer == NEW_LEVEL_TIMOUT_TIMER_DURATION)
		{
			printf("Timeout - display score\r\n");
			prepAndEnterState(display_score_st);
		}
		break;
	case display_score_st:
		if(displayScoreTimer == DISPLAY_SCORE_TIMER_DURATION)
		{
			prepAndEnterState(touch_to_start_st);
		}
		break;
	}
}

void prepAndEnterState(simonControl_states newState)
{
	// State transition
	switch (newState)
	{
	case initial_st:
		break;
	case touch_to_start_st:
		eraseMessage();
		showIntroScreen(false);
		intervalTimer_reset(0);
		intervalTimer_start(0);
		break;
	case wait_for_release_st:
		globals_setSequenceIterationLength(globals_getSequenceIterationLength() + 1);
		break;
	case pause_before_flash_st:
		eraseMessage();
		pauseTimer = 0;
		break;
	case flash_sequence_st:
		simonDisplay_eraseAllButtons();
		flashSequence_enable();
		break;
	case verify_sequence_st:
		flashSequence_disable();
		verifySequence_enable();
		simonDisplay_drawAllButtons();
		break;
	case congrats_st:
		simonDisplay_eraseAllButtons();
		longestSuccessfulSequence = globals_getSequenceLength();
		congratulateUser(false);
		congratsTimer = 0;
		break;
	case touch_for_new_level_st:
		eraseMessage();
		touchForNewLevel(false);
		intervalTimer_reset(0);
		intervalTimer_start(0);
		newLevelTimoutTimer = 0;
		break;
	case display_score_st:
		simonDisplay_eraseAllButtons();
		eraseMessage();
		displayScore(false);
		displayScoreTimer = 0;
		break;
	}
	simonControl_state = newState;
}

void simonControl_test()
{
	display_init();	// We are using the display.
	display_fillScreen(DISPLAY_BLACK);	// Clear the display.

	while(true)
	{
		utils_msDelay(GLOBALS_TIMER_PERIOD);
		simonControl_tick();
		buttonHandler_tick();
		verifySequence_tick();
		flashSequence_tick();
	}
}



