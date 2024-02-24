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

#include "CompileConfig.h"

#ifdef  VL53L0XVersion
#include "S88VL53L0X.h"
#include "NVRAM.h"

S88VL53L0X::S88VL53L0X()
{
	pinStatusData = new struct_pinData[PCA9548_PORT_COUNT];
	SensorTriggerValue = new uint8_t[PCA9548_PORT_COUNT];
	
	SensorReadingIndex = 0;
	for (uint8_t i = 0; i < PCA9548_PORT_COUNT; i++)
	{
		pinStatusData[i].pinHighCounter = 0;
		pinStatusData[i].pinPendingReporting = 1;
	}
}

void S88VL53L0X::ReportAll()
{
	for (int i = 0; i < PCA9548_PORT_COUNT; i++)
	{
		pinStatusData[i].pinPendingReporting = 1;
	}
}

bool S88VL53L0X::ProcessReportItems(const bool& ReportActiveAndInactivePins)
{
	bool ret;
	for (uint8_t i = 0; i < PCA9548_PORT_COUNT; i++)
	{
		if (pinStatusData[i].pinPendingReporting == 1)
		{
			if (pinStatusData[i].pinHighCounter >= 1)
			{
				__SD_CALLBACK_S88REPORT(i, 1, ret);
				if (ret)
				{
					pinStatusData[i].pinPendingReporting = 0;
				}
				return true;
			}
			else
			{
				if (ReportActiveAndInactivePins)
				{
					__SD_CALLBACK_S88REPORT(i, 0, ret);
					if (ret)
					{
						pinStatusData[i].pinPendingReporting = 0;
					}
					return true;
				}
			}
		}
	}
	return false;
}

bool S88VL53L0X::LoadPinValuesFromNVRAM()
{
	for (uint8_t i = 0; i < PCA9548_PORT_COUNT; i++)
	{
		//NVRAM::setSV(CV_ACCESSORY_VL53L0X_SENSORVAL_1 + i, 10 + (i * 1));
		SensorTriggerValue[i] = NVRAM::getSV(SV_ACCESSORY_VL53L0X_SENSORVAL_START + i);
		//Serial.print(F("SensorID: ")); Serial.print(i); Serial.print(F(", TriggerVal: ")); Serial.println(GetSensorTriggerValue(i));
	}
}

void S88VL53L0X::PCA9548PortSelect(const uint8_t& index)
{
	if (index > 7) return;

	Wire.beginTransmission(PCA9548_ADDR);
	Wire.write(1 << index);
	Wire.endTransmission();
}


uint8_t S88VL53L0X::GetSensorCurrentValue(const uint8_t& PinIndex)
{
	//LR_SPRN(F("GSV:"));	LR_SPRN(PinIndex);	LR_SPRN(F(" "));
	if (PinIndex > PCA9548_PORT_COUNT)
		return 0xFF;
	
	uint16_t val = readVL53Sensor(PinIndex);
	//LR_SPRNL(val);
	if(val != 0xFFFF)
	{
		if (val > 9289)
			val = 9289;
			
		return sqrt((val * 7));
	}
	return 0;
}

bool S88VL53L0X::initVL53Sensor(const uint8_t & index)
{
	PCA9548PortSelect(index);
	if (!I2C_CheckPWMDeviceOnAddress(VL53L0X_ADDR))
	{
		return false;
	}

	vl53sensor.setTimeout(250);
	if (!vl53sensor.init())
	{
		return false;
	}
	else
	{
		vl53sensor.startContinuous(VLX_REFRESH_INTERVAL);
		return true;
	}
}

uint16_t S88VL53L0X::GetSensorTriggerValue(const uint8_t &PinIndex)
{
	if (PinIndex > PCA9548_PORT_COUNT)
		return 0xFFFF;

	uint16_t val = SensorTriggerValue[PinIndex];
	return ((val * val) / 7);
}

uint16_t S88VL53L0X::readVL53Sensor(const uint8_t& index)
{
	PCA9548PortSelect(index);

	Wire.beginTransmission(VL53L0X_ADDR);
	if (!Wire.endTransmission())
	{
		//Serial.print("PCA Port #"); Serial.print(index);
		uint16_t val = vl53sensor.readRangeContinuousMillimeters();
		if (vl53sensor.timeoutOccurred())
		{
			//Serial.println(", Timeout#");
			return 0xFFFF;
		}
		else
		{
			//Serial.print(", "); Serial.println(val);
			return val;
		}
	}
	return 0;
}

void S88VL53L0X::Process()
{
	if ((isLoconetGo == 1) || S88ReportingBehavour != S88ReportingOptions::S88ReportOnlyWhenOnGo)
	{

		if ((millis() - previousSendMillis) >= S88_UPDATE_INTERPACK_GAP)
		{
			GetNextItemToReport();
			previousSendMillis = millis();
		}

		if ((millis() - previousRefreshMillis) >= VLX_REFRESH_INTERVAL)
		{
			if (SensorReadingIndex >= PCA9548_PORT_COUNT)
			{
				SensorReadingIndex = 0;
				previousRefreshMillis = millis();
			}
			else
			{
				if (bit_is_set(bt_vl53_enabled_Devices, SensorReadingIndex))
				{
					uint16_t val = readVL53Sensor(SensorReadingIndex);
					if (val == 0xFFFF)
					{
						//Not found or read error
						//try to reinitialize
						initVL53Sensor(SensorReadingIndex);
						//Return 0, to trigger pin high
						val = 0;
					}

					if (val < GetSensorTriggerValue(SensorReadingIndex))
					{
						if (pinStatusData[SensorReadingIndex].pinHighCounter == 0)
						{
							//Serial.print(F("PSensor: ")); Serial.print(SensorReading); Serial.println(F(" - ACTIVE"));
							pinStatusData[SensorReadingIndex].pinPendingReporting = 1;
						}
						pinStatusData[SensorReadingIndex].pinHighCounter = S88_KEEP_ACTIVE_REFRESHINT_COUNT;
					}
					else
					{
						if (pinStatusData[SensorReadingIndex].pinHighCounter > 0)
						{
							if (pinStatusData[SensorReadingIndex].pinHighCounter == 1)
							{
								//Serial.print(F("PSensor: ")); Serial.print(SensorReading); Serial.println(F(" - INACTIVE"));
								pinStatusData[SensorReadingIndex].pinPendingReporting = 1;
							}
							pinStatusData[SensorReadingIndex].pinHighCounter--;
						}
					}
				}
				else
				{
					if (GetSensorTriggerValue(SensorReadingIndex) > 0)
					{					
						if (pinStatusData[SensorReadingIndex].pinHighCounter == 0)
						{							
							//Serial.print(F("PSensor: ")); Serial.print(SensorReading); Serial.println(F(" - ACTIVE"));
							pinStatusData[SensorReadingIndex].pinPendingReporting = 1;
						}
						pinStatusData[SensorReadingIndex].pinHighCounter = S88_KEEP_ACTIVE_REFRESHINT_COUNT;
					}
				}
				SensorReadingIndex++;
			}
		}
	}
}

uint8_t S88VL53L0X::init()
{	
	uint8_t count = 0;
	//Get trigger value setting from NVRAM
	LoadPinValuesFromNVRAM();

	if (!I2C_CheckPWMDeviceOnAddress(PCA9548_ADDR))
	{
		//Serial.print(F("NF I2C 0x"));  Serial.println(PCA9548_ADDR, HEX);
		return 0;
	}	

	for (uint8_t t = 0; t < PCA9548_PORT_COUNT; t++)
	{
		//Serial.print(F("PCA Port #")); Serial.println(t);
		if (initVL53Sensor(t))
		{
			count++;
			//Serial.println(F(" -- Configured"));
			bitSet(bt_vl53_enabled_Devices, t);
		}
	}
		
	return count;
}
#endif