#include "I2CBase.h"
#include "CompileConfig.h"

#ifdef REQUIRES_I2C
	#include <Wire.h>
#endif

bool I2CBase::I2C_CheckPWMDeviceOnAddress(byte I2CAddressToCheck)
{
	#ifdef REQUIRES_I2C
		Wire.beginTransmission(I2CAddressToCheck);
		if(Wire.endTransmission() == 0)
			return true;
	#endif
	return false;
}

bool I2CBase::I2C_Write(const uint8_t& addr, const uint8_t* buffer, const uint8_t& len)
{
	#ifdef REQUIRES_I2C
		Wire.beginTransmission(addr);
		Wire.write(buffer, len);
		if (Wire.endTransmission(true) == 0)
		{
			return true;
		}
	#endif
	return false;
}

