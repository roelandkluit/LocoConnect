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

#ifdef  VL53L0XVersion
#pragma once
#include <Arduino.h>
#include <VL53L0X.h>
#include "CustDefines.h"
#include "CompileConfig.h"
#include "I2CBase.h"
#include "S88Base.h"

#define PCA9548_ADDR 0x74
#define VL53L0X_ADDR 0x29
#define PCA9548_PORT_COUNT 8

#define VLX_REFRESH_INTERVAL 100
#define S88_KEEP_ACTIVE_REFRESHINT_COUNT 10
#define S88_UPDATE_INTERPACK_GAP 5

class S88VL53L0X : I2CBase, public S88Base
{
private:
	unsigned SensorReadingIndex : 4;
	struct struct_pinData
	{
		unsigned pinHighCounter : 6;
		unsigned pinPendingReporting : 1;
		unsigned reserved : 1;
	};	
	VL53L0X vl53sensor;
	struct_pinData* pinStatusData;
	uint8_t* SensorTriggerValue;
	byte bt_vl53_enabled_Devices = 0;
	bool ProcessReportItems(const bool& ReportActiveAndInactivePins) override;
	void PCA9548PortSelect(const uint8_t& index);
	bool initVL53Sensor(const uint8_t& index);
	uint16_t readVL53Sensor(const uint8_t& index);
public:
	S88VL53L0X();
	void ReportAll() override;
	void Process();
	uint8_t init();
	bool LoadPinValuesFromNVRAM();
	uint8_t GetSensorCurrentValue(const uint8_t& PinIndex);
	uint16_t GetSensorTriggerValue(const uint8_t& PinIndex);
};

#endif