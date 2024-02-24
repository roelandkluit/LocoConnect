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
#pragma once
#ifndef _NVRAM_h
#define _NVRAM_h

#include <Arduino.h>
#include <EEPROM.h>
#include "NVRAM_AT24C32.h"

#define END_OF_ARDUINO_MEM E2END
#define EXTERNAL_HEADER_VALUE (uint16_t)0xA4C9 // 2 bytes value

class NVRAM
{
public:
	static uint16_t freeRamMemory();
	static bool InternalEEPROMReady();
	static bool ExternalEEPROMReady();
	static bool InitExternalEEPROM(const uint8_t& DeviceAddress);
	static byte GetExternalMemSize();

	static uint8_t getSV(const uint16_t& SV);
	static byte* EEPROMget(const uint16_t& SV, byte* t, const uint8_t& size);
	static void EEPROMErase(const uint16_t& SV, const uint8_t& size);
	static uint8_t setSV(const uint16_t& SV, const uint8_t& Value);

	static void writeUint16(const uint16_t& SV, const uint16_t& value);
	static uint16_t readUint16(const uint16_t& SV);

	static void writeLNCV(const uint16_t& CV, const uint16_t& value);
	static uint16_t readLNCV(const uint16_t& CV);
	static void SetOnSVChanged(void(*callback)(const uint16_t& SV, const uint8_t& Value)) { __CALLBACK_SV_CHANGED = callback; }

private:
	static NVRAM_AT24C32* I2Ceeprom;
	static void(*__CALLBACK_SV_CHANGED)(const uint16_t& SV, const uint8_t& Value);
	static bool writeEEPROM(const uint16_t& SV, const uint8_t& Value);
	static uint16_t ToExternalEEPROMAddr(const uint16_t& SV);
	static bool CheckExternalMemScope(const uint16_t& ExtMemAddr, const uint8_t& Size = 1);
};

#endif