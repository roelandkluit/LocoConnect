// 
// 
// 

#include "LocalAuxOutput.h"

LocalAuxOutput::LocalAuxOutput(byte pins[], byte size)
{
	pin_ioport = pins;
	capablePinCount = size;
	enabledPinStart = 0;
	enabledPinCount = 0;
}

void LocalAuxOutput::SetOutput(byte index, byte value)
{
	if (index <= enabledPinCount)
	{
		/*Serial.print(F("Output: "));
		Serial.print(index);
		if (value)
		{
			Serial.println(F(" activated"));
		}
		else
		{
			Serial.println(F(" deactivated"));
		}*/
		digitalWrite(enabledPinStart + index, value);
	}
}

void LocalAuxOutput::SetEnabledOutputs(byte pinstart, byte &EnabledPinCount)
{
	if (pinstart > capablePinCount)
	{
		enabledPinCount = 0;
		return;
	}
	enabledPinStart = pinstart;
	enabledPinCount = capablePinCount - enabledPinStart;
	if (enabledPinCount > EnabledPinCount)
		enabledPinCount = EnabledPinCount;

	EnabledPinCount = enabledPinCount;

	/*Serial.print(F("Aux pin "));
	Serial.print(pinstart);
	Serial.print(F(" first available, count: "));
	Serial.println(enabledPinCount);*/

	for (int i = pinstart; i < pinstart + enabledPinCount; i++)
	{
		pinMode(pin_ioport[i], OUTPUT);
	}
}
