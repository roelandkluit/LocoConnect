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

#ifndef _PWMOUTPUTMANAGER_h
#define _PWMOUTPUTMANAGER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "CompileConfig.h"
#include "I2CBase.h"

#define PWM_FREQUENCY 100
#define PIN_ASPECT_COUNT 4
#define REFRESH_INTERVAL 10
#define PWM_FADELEVEL_MAX 4095
#define PWM_FADELEVEL_FULL_NOPWM 4096
#define PWM_DISABLE_SERVO 0xFFFF

#define SERVO_PWM_MIN  150 // This is the 'minimum' pulse length count (out of 4096)
#define SERVO_PWM_MAX  600 // This is the 'maximum' pulse length count (out of 4096)

enum PIN_PWM_Action : byte //Max 0x3F --> 63 (6 bit)
{
	PWM_NONE = 0,
	PWM_IDLE = 1,
	PWM_TURN_OFF = 2,
	PWM_FULL_ON = 3,
	PWM_DIMMED_ON = 4,
	PWM_BLINK = 5,
	PWM_BLINK_INV = 6,
	PWM_RANDOM_ON = 7,
	PWM_SERVO_POS = 8, //Servo pos
	PWM_SERVO_POS_ND = 9, //Servo pos No Disconnect
	PWM_RED_TROUGH_YELLOW = 60
};

enum PIN_ASPECT_INSTRUCTION:byte //6bit 0 t/m 63
{
	None = 0,
	AspectMultibit = 1,
	MultibitOff = 2,
	MultibitOn = 3,
	RedViaYellow = 10,
	Blink = 12,
	BlinkInvert	= 13,
	RandomLed = 14,
	SetServo = 20,
	SetServoND = 21
};

enum PIN_AspectStatus : uint8_t //4 bits for signal(<=15), 3 bits for servo (<=7)
{
	ASPECT_INITIAL = 0,
	ASPECT_TURN_ON = 1,
	ASPECT_ON_WAIT = 2,
	ASPECT_TURN_OFF = 3,
	ASPECT_TURN_OFF_NO_PWM_OUT = 4,
	ASPECT_OFF_WAIT = 5,
};

struct PIN_ActionDataBlink
{
	unsigned FadeTime : 4;
	unsigned HoldTime : 4;
};
struct PIN_ActionDataDimmed
{
	unsigned FadeTime : 4;
	unsigned DimmedValue : 4;
};

union PIN_ActionInputData
{
	byte value;
	PIN_ActionDataBlink BlinkData;
	PIN_ActionDataDimmed DimmedData;
};

struct struct__PinI2CPWMPinAction // 6 + 10 + 8 + 12 + 4 ==> 32 + 8 --> 40
{
	PIN_PWM_Action Action : 6;
	unsigned Reserved : 10;
	PIN_ActionInputData InptData;
	unsigned FadeLevel : 12;
	PIN_AspectStatus PinStatusBits : 4;	
};

struct struct__PinI2CServoPinAction // 6 + 4 + 8 ++ 10 + 4 ==> 32 + 8 --> 40
{
	PIN_PWM_Action Action : 6;
	unsigned ServoTime : 5;
	unsigned TargetPosition : 8;
	unsigned StartPosition : 8;
	unsigned CurrentPosition10bits : 10;
	PIN_AspectStatus PinStatusBits : 3;
};

struct struct__ConfigurationAspectsForPin
{
	PIN_ASPECT_INSTRUCTION Instruction : 6;
	unsigned Reserved : 2;
	unsigned Data0 : 8;
	unsigned Data1 : 8;
};

struct struct__ConfigDCCAddressing
{
	unsigned usePreviousPin : 1;
	unsigned InvertPowerForOutputPin : 1;
	unsigned dccAddress : 14;
};

struct struct__ConfigurationPWMPin
{
	unsigned usePreviousPin : 1;
	unsigned InvertPowerForOutputPin : 1;
	unsigned dccAddress : 14;
	struct__ConfigurationAspectsForPin pinAspectsRed[PIN_ASPECT_COUNT];
	struct__ConfigurationAspectsForPin pinAspectsGreen[PIN_ASPECT_COUNT];
	unsigned reserved : 16;
};

struct struct__Servo_Position_Override
{
	unsigned Index : 6;
	unsigned Exectuted : 1;
	unsigned MoveFast : 1;
	uint8_t Position;
};

#define LAST_LOADED_PIN_NILL 255

class PWMOutputManager : I2CBase
{
private:
	uint8_t DeviceAddressStart = 0;
	uint8_t BitBoardsOnline = 0;
	uint8_t pwmOffSkip = 0;
	unsigned CurrentPinToUpdate:6;
	unsigned initialLoadOfModule:1;
	unsigned UpdateServoThisProcessLoop:1;
	struct__PinI2CPWMPinAction* pinPWMAction;
	struct__ConfigDCCAddressing* addresses;
	struct__ConfigurationPWMPin* lastLoadedConfig;
	uint16_t CalculateFadeStepValue(uint8_t &time);
	void SetPinLedPWM(uint8_t PinIndex, uint16_t PWMvalue);
	void loadCVValues();
	uint8_t lastLoadedAspectPin = LAST_LOADED_PIN_NILL;
	uint16_t maxBrightness = PWM_FADELEVEL_MAX;
	uint8_t pendingFactoryResetPins = 0;
	unsigned long previousRefreshMillis = 0;
	bool ProcessPWMFullON(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin, uint8_t &time);
	void ProcessPWMBlink(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin);
	bool ProcessPWMSleep(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin, uint8_t &time);
	bool ProcessPWMTurnOff(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin, uint8_t &time);
	bool ProcessPWMDimmedOff(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin, uint8_t &time);
	bool ProcessPWMDimmedON(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin, uint8_t &time);
	void ProcessPWMRandomOn(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin);
	void ProcessServo(uint8_t pinIndex, struct__PinI2CPWMPinAction& pin);
	void ProcessAspectChange(struct__ConfigurationAspectsForPin *ascpectconfig, uint8_t &BasePin);
	void ActionRemoveFadeValuesInOffState(struct__PinI2CPWMPinAction& action);
	bool UpdateAspectActionToNew(const uint8_t &PinIndex, const PIN_PWM_Action &NewAction);
	void SetAspectChange(struct__ConfigurationAspectsForPin &aspect, uint8_t &BasePin, uint8_t &MaxPinsToControl);
	bool ExecuteAspectForPin(uint8_t &PinID, const uint8_t &Direction);
	void SetPinServoPWM(const uint8_t &PinIndex, const uint16_t &PWM_value);
	uint8_t GetAspectBasePin(uint8_t &currentPin);
	uint8_t MaxPinsToControl(uint8_t &BasePin);
	void SetUint8ToCurrentServoPosition(const uint8_t &currentPos, struct__PinI2CServoPinAction* ServoPin);
	void SetFloatToCurrentServoPosition(const float &currentPos, struct__PinI2CServoPinAction* ServoPin);
	void UpdateServoCurrentPosition(struct__PinI2CServoPinAction* pin, float& value);
	uint8_t GetCurrentServoPositionAsUint8(struct__PinI2CServoPinAction* ServoPin);
	void ProcessOutputPin(uint8_t pin);

	//I2C Functions
	bool I2C_SetPinPWMValue(const uint8_t& addr, const uint8_t& PinIndex, const uint16_t& onTime, const uint16_t& offTime);
	bool I2C_WriteRegister(const uint8_t& addr, const uint8_t& regaddr, const uint8_t& value);
	void I2C_DeviceRestart(const uint8_t& addr);
	void I2C_SetPWMFreq(const uint8_t& addr);
	uint8_t I2C_ReadRegister(const uint8_t& addr, const uint8_t& regaddr);
public:
	void SetFactoryDefaults() { pendingFactoryResetPins = MAX_NUMBER_OF_PWM_OUTPUT_PINS; }
	void CVValueHasBeenChanged();
	PWMOutputManager();
	~PWMOutputManager();
	//static bool I2C_CheckPWMDeviceOnAddress(byte I2CAddressToCheck);
	uint8_t GetOnlineBoardsBitmaskValue();
	void ChangeServoPosition(struct__Servo_Position_Override& PositionInfo);
	void init(const uint8_t &addrPWMModuleStart);
	void InitAllPinsToRed();
	bool CheckRange(const uint8_t &LedID);
	void NotifyAccTurnoutOutput(const uint16_t &Addr, const uint8_t &Direction);
	void process();
	//uint16_t NextAddrToRetrievePos();
};

#endif

