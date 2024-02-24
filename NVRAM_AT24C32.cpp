// 
// 
// 
#include "NVRAM_AT24C32.h"
#include <Wire.h>

//  I2C buffer needs max 2 bytes for EEPROM address
//  1 byte for EEPROM register address is available in transmit buffer
#if defined(ESP32) || defined(ESP8266)
#define I2C_BUFFERSIZE   128
#else
#define I2C_BUFFERSIZE   30   //  AVR, STM
#endif

#define I2C_WRITEDELAY   5000
#define I2C_COMPARE_SIZE 10

NVRAM_AT24C32::NVRAM_AT24C32(const uint8_t& DeviceAddress)
{
    _deviceAddress = DeviceAddress;
}

bool NVRAM_AT24C32::CheckSizeOk()
{
    bool isModifiedFirstSector = false;
    bool dataIsDifferent = false;
    bool ret = false;

    byte* dataBlock0 = new byte[I2C_COMPARE_SIZE];
    byte* dataBlock1 = new byte[I2C_COMPARE_SIZE];

    if (ReadBlock(0, dataBlock0, I2C_COMPARE_SIZE) == I2C_COMPARE_SIZE)
    {
        for (uint8_t pos = 0; pos < I2C_COMPARE_SIZE; pos++)
        {
            if (dataIsDifferent || pos == 0)
            {
                //ignore futher comparison if dataFirstBytes is not the same in buffer
                //Ignore first byte
            }
            else if (dataBlock0[pos - 1] != dataBlock0[pos])
            {
                dataIsDifferent = true;
            }

            if (dataBlock0[pos] != 0xFF && dataBlock0[pos] != 0x00)
            {
                //Default dataFirstBytes value is 0xFF or 0x00
                isModifiedFirstSector = true;
            }

            if (dataIsDifferent && isModifiedFirstSector)
                break;
        }

        if ((!isModifiedFirstSector) || (!dataIsDifferent))
        {
            if (WriteByte(0, 0x7B) == 0)
            {
                ReadBlock(0, dataBlock0, I2C_COMPARE_SIZE);
            }
        }

        if (ReadBlock((I2C_DEVICESIZE_24LC32 / 2), dataBlock1, I2C_COMPARE_SIZE) == I2C_COMPARE_SIZE)
        {
            if (memcmp(dataBlock0, dataBlock1, I2C_COMPARE_SIZE) != 0)
            {
                ret = true;
            }
        }
    }

    delete[] dataBlock0;
    delete[] dataBlock1;
    return ret;
}

bool NVRAM_AT24C32::CheckReady()
{
    //  Wait until EEPROM gives ACK again.
    do
    {
        if (I2C_CheckPWMDeviceOnAddress(_deviceAddress))
            return true;
        yield();     //  For OS scheduling
    } while ((micros() - _lastWrite) <= I2C_WRITEDELAY);
    return false;
}

NVRAM_AT24C32* NVRAM_AT24C32::Initialize(const uint8_t& DeviceAddress)
{
    if ((!I2C_CheckPWMDeviceOnAddress(DeviceAddress)) || (!I2C_CheckPWMDeviceOnAddress(DeviceAddress + 8)))
        return NULL;

    NVRAM_AT24C32* dev = new NVRAM_AT24C32(DeviceAddress);
    if (dev->CheckSizeOk())
    {
        return dev;
    }
    else
    {
        delete dev;
        return NULL;
    }
}

uint8_t NVRAM_AT24C32::ReadByte(const uint16_t& memoryAddress)
{
    byte bTmp = 0;
    ReadBlock(memoryAddress, &bTmp, 1);
    return bTmp;
}

uint8_t NVRAM_AT24C32::WriteByte(const uint16_t& memoryAddress, const uint8_t& data)
{
    return WriteBlock(memoryAddress, &data, 1);
}

uint8_t NVRAM_AT24C32::WriteBlock(const uint16_t& memoryAddress, const uint8_t* buffer, const uint8_t& length)
{
    if (!BeginTransmission(memoryAddress))
        return 0;

    uint8_t bufSize = AdjustBuffLenght(length);
    Wire.write(buffer, bufSize);
    uint8_t rv = Wire.endTransmission();

    //Serial.print("Write:"); Serial.println(rv);
    _lastWrite = micros();

    return rv;
}

bool NVRAM_AT24C32::BeginTransmission(const uint16_t& memoryAddress)
{
    if (!CheckReady())
        return false;

    Wire.beginTransmission(_deviceAddress);
    Wire.write(highByte(memoryAddress));
    Wire.write(lowByte(memoryAddress));
    return true;
}

uint8_t NVRAM_AT24C32::AdjustBuffLenght(const uint8_t& bufLenght)
{
    if (bufLenght > I2C_BUFFERSIZE)
        return I2C_BUFFERSIZE;
    else
        return bufLenght;
}

//  returns bytes read
uint8_t NVRAM_AT24C32::ReadBlock(const uint16_t& memoryAddress, uint8_t* buffer, const uint8_t& length)
{
    if (!BeginTransmission(memoryAddress))
        return 0;

    if (Wire.endTransmission() != 0)
        return 0;

    uint8_t bufSize = AdjustBuffLenght(length);
    uint8_t readBytes = 0;
    uint8_t cnt = 0;

    //  readBytes will always be equal or smaller to length      
    readBytes = Wire.requestFrom(_deviceAddress, bufSize);

    while (cnt < readBytes)
    {
        buffer[cnt++] = Wire.read();
    }
    return readBytes;
}