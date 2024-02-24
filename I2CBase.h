#include "CompileConfig.h"
#include <Arduino.h>

#pragma once
class I2CBase
{
public:
	static bool I2C_CheckPWMDeviceOnAddress(byte I2CAddressToCheck);
	static bool I2C_Write(const uint8_t& addr, const uint8_t* buffer, const uint8_t& len);
};
