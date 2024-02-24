// NVRAM_AT24C32.h

#ifndef _NVRAM_AT24C32_h
#define _NVRAM_AT24C32_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "I2CBase.h"
#define I2C_DEVICESIZE_24LC32        4096
#define I2C_DEVICESIZE_24LC08		 1024

#pragma once
class NVRAM_AT24C32 : I2CBase
{
private:
	NVRAM_AT24C32(const uint8_t& DeviceAddress);
	bool CheckSizeOk();
	bool CheckReady();
	uint8_t _deviceAddress = 0;
	uint32_t _lastWrite = 0;  //  for waitEEReady
	bool BeginTransmission(const uint16_t& memoryAddress);
	uint8_t AdjustBuffLenght(const uint8_t& bufLenght);
public:
	uint8_t GetDeviceSizeType() { return I2C_DEVICESIZE_24LC32 / I2C_DEVICESIZE_24LC08; };
	uint32_t GetDeviceSize() { return I2C_DEVICESIZE_24LC32; };
	static NVRAM_AT24C32* Initialize(const uint8_t& DeviceAddress);
	uint8_t ReadByte(const uint16_t& memoryAddress);
	uint8_t WriteByte(const uint16_t& memoryAddress, const uint8_t& data);
	uint8_t WriteBlock(const uint16_t& memoryAddress, const uint8_t* buffer, const uint8_t& length);
	uint8_t ReadBlock(const uint16_t& memoryAddress, uint8_t* buffer, const uint8_t& length);
};

#endif