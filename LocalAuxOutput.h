// LocalAuxOutput.h

#ifndef _LOCALAUXOUTPUT_h
#define _LOCALAUXOUTPUT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class LocalAuxOutput
{
 private:
	 byte* pin_ioport;
	 byte enabledPinCount = 0;
	 byte enabledPinStart = 0;
	 byte capablePinCount = 0;
 public:
	LocalAuxOutput(byte pins[], byte size);
	void SetOutput(byte index, byte value);
	void SetEnabledOutputs(byte pinstart, byte &EnabledPinCount);
};

#endif

