/*
"Commons Clause" License Condition v1.0

The Software is provided to you by the Licensor under the License, as defined below, subject to the following condition.

Without limiting other conditions in the License, the grant of rights under the License will not include, and the License does not grant to you, the right to Sell the Software.

For purposes of the foregoing, "Sell" means practicing any or all of the rights granted to you under the License to provide to third parties,
for a fee or other consideration (including without limitation fees for hosting or consulting/ support services related to the Software),
a product or service whose value derives, entirely or substantially, from the functionality of the Software.
Any license notice or attribution required by the License must also include this Commons Clause License Condition notice.

Software: LocoConnect
License: GPLv3
Licensor: Roeland Kluit

Copyright (C) 2024 Roeland Kluit - v0.6 Februari 2024 - All rights reserved -

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The Software is provided to you by the Licensor under the License,
as defined, subject to the following condition.

Without limiting other conditions in the License, the grant of rights
under the License will not include, and the License does not grant to
you, the right to Sell the Software.

For purposes of the foregoing, "Sell" means practicing any or all of
the rights granted to you under the License to provide to third
parties, for a fee or other consideration (including without
limitation fees for hosting or consulting/ support services related
to the Software), a product or service whose value derives, entirely
or substantially, from the functionality of the Software.
Any license notice or attribution required by the License must also
include this Commons Clause License Condition notice.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "NVRAM.h"
#include "CompileConfig.h"

void(*NVRAM::__CALLBACK_SV_CHANGED)(const uint16_t&, const uint8_t&) = NULL;
#if defined(REQUIRES_I2C)
    NVRAM_AT24C32* NVRAM::I2Ceeprom = NULL;
#endif

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void* __brkval;

uint16_t NVRAM::freeRamMemory()
{
    int free_memory;

    if ((int)__brkval == 0)
        free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else
        free_memory = ((int)&free_memory) - ((int)__brkval);

    return free_memory;
}

//EEPROM Ready and constructor functions

bool NVRAM::InternalEEPROMReady()
{
    return eeprom_is_ready();
}

bool NVRAM::ExternalEEPROMReady()
{
    //Serial.print(F("EepReady:"));
    //Serial.println(I2Ceeprom != NULL);
    return I2Ceeprom != NULL;
}

byte NVRAM::GetExternalMemSize()
{
    #if defined(REQUIRES_I2C)
        if (I2Ceeprom == NULL)
            return 0;

        return I2Ceeprom->GetDeviceSizeType();
    #else
        return 0;
    #endif
}

bool NVRAM::InitExternalEEPROM(const uint8_t& DeviceAddress)
{
    #if defined(REQUIRES_I2C)
        if (I2Ceeprom != NULL)
            return true;

        I2Ceeprom = NVRAM_AT24C32::Initialize(DeviceAddress);

        if (I2Ceeprom == NULL)
            return false;

        uint16_t bHeader = 0;
        uint8_t l = I2Ceeprom->ReadBlock(0, reinterpret_cast<byte*>(&bHeader), sizeof(uint16_t));
        if (bHeader != EXTERNAL_HEADER_VALUE)
        {
            //Todo Erase??
            bHeader = EXTERNAL_HEADER_VALUE;
            I2Ceeprom->WriteBlock(0, reinterpret_cast<byte*>(&bHeader), sizeof(uint16_t));
        }
        return true;
    #else
        return false;
    #endif

}

bool NVRAM::CheckExternalMemScope(const uint16_t& ExtMemAddr, const uint8_t& Size)
{
    #if defined(REQUIRES_I2C)
        if (I2Ceeprom == NULL)
            return false;

        if (Size == 0)
            //Invalid size
            return false;

        if (I2Ceeprom->GetDeviceSize() >= (ExtMemAddr + Size))
        {
            //Addressing within external EEPROM module range
            return true;
        }
        else
        {
            //Addressing NOT within external EEPROM module range
            return false;
        }
    #else
        return false;
    #endif
}

uint16_t NVRAM::ToExternalEEPROMAddr(const uint16_t& SV)
{
    //Check if SV range is correct for external device
    if (SV < END_OF_ARDUINO_MEM)
    {
        //Serial.println(F("RangeError"));
        return 0xFFFF;
    }

    return ((SV - END_OF_ARDUINO_MEM) + sizeof(EXTERNAL_HEADER_VALUE));
}

//SV Functions
uint8_t NVRAM::getSV(const uint16_t& SV)
{
    if (SV <= END_OF_ARDUINO_MEM)
    {
        return EEPROM.read(SV);
    }
    else if (ExternalEEPROMReady())
    {
        uint16_t ext = ToExternalEEPROMAddr(SV);
        if (CheckExternalMemScope(ext))
        {
            return I2Ceeprom->ReadByte(ext);
        }
    }
    return 0;
}

byte* NVRAM::EEPROMget(const uint16_t& SV, byte* t, const uint8_t& size)
{
    //Serial.print(F("ERG"));

    if (SV <= END_OF_ARDUINO_MEM)
    {
        //Serial.print(F("UM:"));
        //Serial.println(SV);

        for (uint8_t count = 0; count < size; count++)
        {
            t[count] = getSV(SV + count);
        }
        return t;
    }
    else if (ExternalEEPROMReady())
    {
        uint16_t ext = ToExternalEEPROMAddr(SV);
        //Serial.print(F("ReadFrom:")); Serial.println(ext);

        if (CheckExternalMemScope(ext, size))
        {
            uint8_t rSize = I2Ceeprom->ReadBlock(ext, t, size);
            //Serial.print(F("rSize:")); Serial.println(rSize);

            if (rSize == size)
            {
                return t;
            }
        }
        else
        {
            //Serial.println(F("OOS"));
        }
    }
    else
    {
        //Serial.println(F("ENR"));
    }
    return 0;
}

bool NVRAM::writeEEPROM(const uint16_t& SV, const uint8_t& Value)
{
    //Check if in range for Arduino
    if (SV <= END_OF_ARDUINO_MEM)
    {
        EEPROM.write(SV, Value);
        return true;
    }
    else if (ExternalEEPROMReady())
    {
        uint16_t ext = ToExternalEEPROMAddr(SV);
        //Serial.print(F("WriteTo:")); Serial.println(ext);
        if (CheckExternalMemScope(ext))
        {
            if (I2Ceeprom->WriteByte(ext, Value))
            {
                return true;
            }
        }
    }
    return false;
}

//Set SV with Callback notification
uint8_t NVRAM::setSV(const uint16_t& SV, const uint8_t& Value)
{
    if (getSV(SV) != Value)
    {
        writeEEPROM(SV, Value);

        if (__CALLBACK_SV_CHANGED != NULL)
            __CALLBACK_SV_CHANGED(SV, Value);
    }

    return getSV(SV);
}

void NVRAM::EEPROMErase(const uint16_t& SV, const uint8_t& size)
{
    for (uint8_t count = 0; count < size; count++)
    {
        setSV((count + SV), 0);
    }
}

//SV 16 bits reading functions
void NVRAM::writeUint16(const uint16_t& SV, const uint16_t& value)
{
    setSV(SV, value);
    setSV(SV + 1, value >> 8);
}

uint16_t NVRAM::readUint16(const uint16_t& SV) {
    int val;

    val = (getSV(SV + 1) << 8);
    val |= getSV(SV);

    return val;
}

//Loconet CV addressing functions (CVaddr * 2 ==> SVaddr)
void NVRAM::writeLNCV(const uint16_t& CV, const uint16_t& value)
{
    writeUint16(CV * 2, value);
}

uint16_t NVRAM::readLNCV(const uint16_t& CV)
{
    return readUint16(CV * 2);
}
