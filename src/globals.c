#include "globals.h"

uint8_t globalsSequence[GLOBALS_MAX_FLASH_SEQUENCE];
uint16_t globalsSeqLength;
uint16_t globalsIterLength;

// This is the length of the complete sequence at maximum length.
// You must copy the contents of the sequence[] array into the global variable that you maintain.
// Do not just grab the pointer as this will fail.
void globals_setSequence(const uint8_t sequence[], uint16_t length)
{
	for(int i = 0; i < length; i++)
	{
		globalsSequence[i] = sequence[i];
	}
	globalsSeqLength = length;
}

// This returns the value of the sequence at the index.
uint8_t globals_getSequenceValue(uint16_t index)
{
	return globalsSequence[index];
}

// Retrieve the sequence length.
uint16_t globals_getSequenceLength()
{
	return globalsSeqLength;
}

// This is the length of the sequence that you are currently working on.
void globals_setSequenceIterationLength(uint16_t length)
{
	globalsIterLength = length;
}

// This is the length of the sequence that you are currently working on (not the maximum length but the interim length as
// the use works through the pattern one color at a time.
uint16_t globals_getSequenceIterationLength()
{
	return globalsIterLength;
}
