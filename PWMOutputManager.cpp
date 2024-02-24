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
#include "PWMOutputManager.h"
#include "NVRAM.h"
#include "CustDefines.h"
#include "Wire.h"

#define timeDefinitionSixstOfASeconds 6
#define ServoDelayFactor 5
#define pinRefreshCountPerSeconds (1000 / REFRESH_INTERVAL)
#define PWMFadeTimeCalculationBase (PWM_FADELEVEL_MAX * timeDefinitionSixstOfASeconds) / pinRefreshCountPerSeconds

#define PCA9685_PRESCALE 0xFE     /**< Prescaler for PWM output frequency */
#define PCA9685_PRESCALE_VALUE 0x41
#define PCA9685_MODE1 0x00      /**< Mode Register 1 */
#define PCA9685_MODE1_AUTOINCREASE 0x20      /**< Auto-Increment enabled */
#define PCA9685_MODE1_RESTART 0x80 /**< Restart enabled */
#define PCA9685_MODE1_SLEEP 0x10   /**< Low power mode. Oscillator offTime */
#define PCA9685_LED0 0x06  //First byte, of first led

PWMOutputManager::PWMOutputManager()
{
	pinPWMAction = new struct__PinI2CPWMPinAction[MAX_NUMBER_OF_PWM_OUTPUT_PINS]; //40 bits, 5 bytes
	addresses = new struct__ConfigDCCAddressing[MAX_NUMBER_OF_PWM_OUTPUT_PINS]; //16 bits, 2 bytes
	lastLoadedConfig = new struct__ConfigurationPWMPin;
	memset(pinPWMAction, 0, sizeof(struct__PinI2CPWMPinAction) * MAX_NUMBER_OF_PWM_OUTPUT_PINS);
	memset(addresses, 0, sizeof(struct__ConfigDCCAddressing) * MAX_NUMBER_OF_PWM_OUTPUT_PINS);
	memset(lastLoadedConfig, 0, sizeof(struct__ConfigurationPWMPin));
	CurrentPinToUpdate = 0;
	previousRefreshMillis = 0;
	UpdateServoThisProcessLoop = 0;
}

PWMOutputManager::~PWMOutputManager()
{
}

uint8_t PWMOutputManager::GetOnlineBoardsBitmaskValue()
{
	return BitBoardsOnline;
}

void PWMOutputManager::ChangeServoPosition(struct__Servo_Position_Override& PositionInfo)
{
	PositionInfo.Exectuted = 0;
	if (!CheckRange(PositionInfo.Index))
		return;

	switch (pinPWMAction[PositionInfo.Index].Action)
	{
		case PIN_PWM_Action::PWM_SERVO_POS:
		case PIN_PWM_Action::PWM_SERVO_POS_ND:
			break;
		default:
			return;
	}

	struct__PinI2CServoPinAction* pSvoPinAction = reinterpret_cast<struct__PinI2CServoPinAction*>(&pinPWMAction[PositionInfo.Index]);
	pSvoPinAction->TargetPosition = PositionInfo.Position;
	pSvoPinAction->PinStatusBits = PIN_AspectStatus::ASPECT_TURN_ON;
	if(PositionInfo.MoveFast)
		SetUint8ToCurrentServoPosition(pSvoPinAction->TargetPosition, pSvoPinAction);
	else
		pSvoPinAction->StartPosition = GetCurrentServoPositionAsUint8(pSvoPinAction);
	PositionInfo.Exectuted = 1;
}

void PWMOutputManager::init(const uint8_t &addrPWMModuleStart)
{	
	//LR_SPRN(F("ModEna:"));	LR_SPRNL(sizeof(pwm) / sizeof(pwm[0]));

	DeviceAddressStart = addrPWMModuleStart;

	for (uint8_t cntr = 0; cntr < MAX_NUMBER_OF_I2C_PWM_BOARDS; cntr++)
	{
		if (I2C_CheckPWMDeviceOnAddress(addrPWMModuleStart + cntr))
		{
			I2C_DeviceRestart(addrPWMModuleStart + cntr);
			I2C_SetPWMFreq(addrPWMModuleStart + cntr);
			bitSet(BitBoardsOnline, 7 - cntr);
		}
	}

	loadCVValues();

	for (uint8_t i = 0; i < MAX_NUMBER_OF_PWM_OUTPUT_PINS; i++)
	{
		SetPinLedPWM(i, 0);
	}
	InitAllPinsToRed();
}

void PWMOutputManager::InitAllPinsToRed()
{
	initialLoadOfModule = 1;
	for (uint8_t PinID = 0; PinID < MAX_NUMBER_OF_PWM_OUTPUT_PINS; PinID++)
	{
		if (addresses[PinID].usePreviousPin == 0 && addresses[PinID].dccAddress != 0)
		{
			ExecuteAspectForPin(PinID, 0);
		}
	}
	initialLoadOfModule = 0;
}

void PWMOutputManager::loadCVValues()
{
	if (pendingFactoryResetPins != 0)
		return;
	
	uint8_t pins = MAX_NUMBER_OF_PWM_OUTPUT_PINS;	
	for (uint8_t PinID = 0; PinID < MAX_NUMBER_OF_PWM_OUTPUT_PINS; PinID++)
	{
		uint16_t addrPos = SV_ACCESSORY_DECODER_ASPECTS_DATA_START + (sizeof(struct__ConfigurationPWMPin) * PinID);
		NVRAM::EEPROMget(addrPos, reinterpret_cast<byte*>(&addresses[PinID]), sizeof(struct__ConfigDCCAddressing));
		//LR_SPRN(PinID); LR_SPRN(F(" ")); LR_SPRNL(addresses[PinID].dccAddress);
	}
}

bool PWMOutputManager::CheckRange(const uint8_t &LedID)
{
	if (LedID < MAX_NUMBER_OF_PWM_OUTPUT_PINS)
		return true;
	return false;
}

uint8_t PWMOutputManager::GetAspectBasePin(uint8_t &currentPin)
{
	for (uint8_t i = currentPin; i != 0; i--)
	{
		LR_SPRN(F("ABP-P: ")); LR_SPRN(i + 1); LR_SPRN(F(" usP: ")); LR_SPRNL(addresses[i].usePreviousPin);
		
		if (addresses[i].usePreviousPin != 1)
		{
			LR_SPRN(F("UP: ")); LR_SPRNL(i + 1);			
			return i;
		}
	}
	LR_SPRNL(F("ABP-P: 0"));	
	return 0;
}

uint8_t PWMOutputManager::MaxPinsToControl(uint8_t &BasePin)
{
	for (uint8_t i = 1; i <= 8; i++)
	{
		if (CheckRange(BasePin + i))
		{
			if (addresses[BasePin + i].usePreviousPin == 0)
				return i;
		}
		else
		{
			//End of all available pins
			return i;
		}
	}
	return 8;
}

void PWMOutputManager::ProcessAspectChange(struct__ConfigurationAspectsForPin *ascpectconfig, uint8_t &BasePin)
{
	uint8_t maxControl = MaxPinsToControl(BasePin);
	for (uint8_t i = 0; i < PIN_ASPECT_COUNT; i++)
	{
		if (ascpectconfig[i].Instruction != 0)
		{
			LR_SPRN(F("P: ")); LR_SPRN(BasePin); LR_SPRN(F(" Ap: ")); LR_SPRN(i); LR_SPRN(F(" Itr: ")); LR_SPRN(ascpectconfig[i].Instruction);	LR_SPRN(F(" D1: ")); LR_SPRN(ascpectconfig[i].Data1); LR_SPRN(F(" D0: ")); LR_SPRNL(ascpectconfig[i].Data0);			
			SetAspectChange(ascpectconfig[i], BasePin, maxControl);
		}
	}
}

void PWMOutputManager::CVValueHasBeenChanged()
{
	lastLoadedAspectPin = LAST_LOADED_PIN_NILL;
	memset(lastLoadedConfig, 0, sizeof(struct__ConfigurationPWMPin));
	loadCVValues();
}

void PWMOutputManager::ActionRemoveFadeValuesInOffState(struct__PinI2CPWMPinAction &action)
{
	switch (action.Action)
	{
	case PIN_PWM_Action::PWM_BLINK:
	case PIN_PWM_Action::PWM_BLINK_INV:
	case PIN_PWM_Action::PWM_RANDOM_ON:
		if (action.PinStatusBits == ASPECT_OFF_WAIT)
			action.FadeLevel = 0;
		else if (action.PinStatusBits == ASPECT_TURN_OFF_NO_PWM_OUT)
			action.FadeLevel = 0;
		break;
	}
}

bool PWMOutputManager::UpdateAspectActionToNew(const uint8_t &PinIndex, const PIN_PWM_Action &NewAction)
{
	if (pinPWMAction[PinIndex].Action != NewAction)
	{
		//LR_SPRN(F("- AspChange pin:")); LR_SPRN(PinIndex); LR_SPRN(F(" fade:")); LR_SPRN(pinPWMAction[PinIndex].FadeLevel);	LR_SPRN(F(" val: ")); LR_SPRNL(NewAction);
		ActionRemoveFadeValuesInOffState(pinPWMAction[PinIndex]);
		pinPWMAction[PinIndex].Action = NewAction;
		switch (NewAction)
		{
		case PWM_SERVO_POS: //Do not update PinStatusBits for Servo, it is using alternative pinAction struct
			break;
		default:
			pinPWMAction[PinIndex].PinStatusBits = PIN_AspectStatus::ASPECT_INITIAL;
			break;
		}
		return true;
	}
	return false;
}

void PWMOutputManager::SetAspectChange(struct__ConfigurationAspectsForPin &aspect, uint8_t &BasePin, uint8_t &MaxPinsToControl)
{
	switch (aspect.Instruction)
	{
	case PIN_ASPECT_INSTRUCTION::None:
		break;
	case PIN_ASPECT_INSTRUCTION::Blink:
		if (aspect.Data0 > MaxPinsToControl)
		{
			//LR_SPRNL("OOB-pinindex");
			return;
		}
		UpdateAspectActionToNew(BasePin + aspect.Data0, PIN_PWM_Action::PWM_BLINK);
		pinPWMAction[BasePin + aspect.Data0].InptData.value = aspect.Data1;		
		break;
	case PIN_ASPECT_INSTRUCTION::BlinkInvert:
		if (aspect.Data0 > MaxPinsToControl)
		{
			//LR_SPRNL("OOB-pinindex");
			return;
		}
		UpdateAspectActionToNew(BasePin + aspect.Data0, PIN_PWM_Action::PWM_BLINK_INV);
		pinPWMAction[BasePin + aspect.Data0].InptData.value = aspect.Data1;
		break;
	case PIN_ASPECT_INSTRUCTION::RandomLed:
		if (aspect.Data0 > MaxPinsToControl)
		{
			//LR_SPRNL("OOB-pinindex");
			return;
		}
		UpdateAspectActionToNew(BasePin + aspect.Data0, PIN_PWM_Action::PWM_RANDOM_ON);
		pinPWMAction[BasePin + aspect.Data0].InptData.value = aspect.Data1;
		break;

	case PIN_ASPECT_INSTRUCTION::AspectMultibit:
		for (uint8_t i = 0; i < MaxPinsToControl; i++)
		{
			//Loop through each bit and check if it needs to be onTime or offTime
			//LR_SPRN("Pin: "); LR_SPRN(i); LR_SPRN(" Ad0: "); LR_SPRNL(bitRead(aspect.Data0, i));
			if (bitRead(aspect.Data0, i))
			{
				UpdateAspectActionToNew(BasePin + i, PIN_PWM_Action::PWM_FULL_ON);
				pinPWMAction[i + BasePin].InptData.value = aspect.Data1;
			}
			else
			{
				UpdateAspectActionToNew(BasePin + i, PIN_PWM_Action::PWM_TURN_OFF);
				pinPWMAction[i + BasePin].InptData.value = aspect.Data1;
			}
		}
		break;
	case PIN_ASPECT_INSTRUCTION::MultibitOff:
		for (uint8_t i = 0; i < MaxPinsToControl; i++)
		{
			//LR_SPRN("Pin: "); LR_SPRN(i); LR_SPRN(" Ad0: "); LR_SPRNL(bitRead(aspect.Data0, i));
			if (bitRead(aspect.Data0, i))
			{
				UpdateAspectActionToNew(BasePin + i, PIN_PWM_Action::PWM_TURN_OFF);
				pinPWMAction[i + BasePin].InptData.value = aspect.Data1;
			}
		}
		break;
	case PIN_ASPECT_INSTRUCTION::MultibitOn:
		for (uint8_t i = 0; i < MaxPinsToControl; i++)
		{
			//LR_SPRN("Pin: "); LR_SPRN(i); LR_SPRN(" Ad0: "); LR_SPRNL(bitRead(aspect.Data0, i));
			if (bitRead(aspect.Data0, i))
			{
				UpdateAspectActionToNew(BasePin + i, PIN_PWM_Action::PWM_FULL_ON);
				pinPWMAction[i + BasePin].InptData.value = aspect.Data1;
			}
		}
		break;
	case PIN_ASPECT_INSTRUCTION::RedViaYellow:
		//LR_SPRNL(BasePin);	LR_SPRNL(MaxPinsToControl); LR_SPRNL(aspect.Data0); LR_SPRNL(aspect.Data1);
		if (MaxPinsToControl >= aspect.Data0 + 3)
		{
			uint8_t RedPin = aspect.Data0 + BasePin;
			uint8_t GreenPin = aspect.Data0 + BasePin + 1;
			uint8_t YellowPin = aspect.Data0 + BasePin + 2;
			//LR_SPRNL(pinPWMAction[GreenPin].Action);

			if (pinPWMAction[YellowPin].Action != PIN_PWM_Action::PWM_RED_TROUGH_YELLOW)
			{
				switch (pinPWMAction[GreenPin].Action)
				{
				case PIN_PWM_Action::PWM_BLINK:
				case PIN_PWM_Action::PWM_FULL_ON:
				case PIN_PWM_Action::PWM_BLINK_INV:
				case PIN_PWM_Action::PWM_DIMMED_ON:
					//LR_SPRNL(F("SetPinRedYellow"));
					UpdateAspectActionToNew(YellowPin, PIN_PWM_Action::PWM_RED_TROUGH_YELLOW);
					UpdateAspectActionToNew(GreenPin, PIN_PWM_Action::PWM_TURN_OFF);
					pinPWMAction[YellowPin].PinStatusBits = PIN_AspectStatus::ASPECT_INITIAL;
					pinPWMAction[YellowPin].InptData.value = aspect.Data1;
					pinPWMAction[RedPin].InptData.value = pinPWMAction[YellowPin].InptData.BlinkData.FadeTime; //Already set time, used later when toggeling over Yellow
					pinPWMAction[GreenPin].InptData.value = pinPWMAction[YellowPin].InptData.BlinkData.FadeTime; //Set fade out time
					break;
				default:
					//LR_SPRNL(F("Nofade"));
					PIN_ActionInputData Uid; Uid.value = aspect.Data1; // The aspect data contains hold and fade time, when switching to red directly. No hold time is used.
					UpdateAspectActionToNew(RedPin, PIN_PWM_Action::PWM_FULL_ON);
					UpdateAspectActionToNew(YellowPin, PIN_PWM_Action::PWM_TURN_OFF);
					pinPWMAction[RedPin].InptData.value = Uid.BlinkData.FadeTime;
					pinPWMAction[YellowPin].InptData.value = Uid.BlinkData.FadeTime;
					break;
				}
			}
		}
		/*else
		{
			LR_SPRNL(F("Pincount mismatch"));
		}*/
		break;
	case PIN_ASPECT_INSTRUCTION::SetServo:
	case PIN_ASPECT_INSTRUCTION::SetServoND:
	{
		byte pinIndex = ((aspect.Data0 & 0xE0) >> 5);
		byte pinTime = (aspect.Data0 & 0x1F);

		//LR_SPRN(F("SvoPinIndex: ")); LR_SPRNL(pinIndex); LR_SPRN(F("SvoTime: "));	LR_SPRNL(pinTime);
		if (pinIndex > MaxPinsToControl)
		{
			//LR_SPRNL(F("SOOB-pinindex"));
			return;
		}

		if (initialLoadOfModule==1)
		{
			//LR_SPRNL(F("Skip Initial for Servo"));
			return;
		}

		struct__PinI2CServoPinAction* pSvoPinAction = reinterpret_cast<struct__PinI2CServoPinAction*>(&pinPWMAction[BasePin + pinIndex]);

		if (aspect.Instruction == PIN_ASPECT_INSTRUCTION::SetServo)
		{
			if (UpdateAspectActionToNew(BasePin + pinIndex, PIN_PWM_Action::PWM_SERVO_POS))
			{
				pSvoPinAction->PinStatusBits = PIN_AspectStatus::ASPECT_INITIAL;
			}
		}
		else if (aspect.Instruction == PIN_ASPECT_INSTRUCTION::SetServoND)
		{
			if (UpdateAspectActionToNew(BasePin + pinIndex, PIN_PWM_Action::PWM_SERVO_POS_ND))
			{
				pSvoPinAction->PinStatusBits = PIN_AspectStatus::ASPECT_INITIAL;
			}
		}

		if (pSvoPinAction->PinStatusBits != PIN_AspectStatus::ASPECT_INITIAL)
		{
			pSvoPinAction->PinStatusBits = PIN_AspectStatus::ASPECT_TURN_ON;
		}

		if (pSvoPinAction->TargetPosition != aspect.Data1)
		{
			pSvoPinAction->StartPosition = pSvoPinAction->TargetPosition;
			pSvoPinAction->TargetPosition = aspect.Data1;
			pSvoPinAction->ServoTime = pinTime;
		}

		//LR_SPRN(F("SvoAction: ")); LR_SPRNL(pSvoPinAction->Action); LR_SPRN(F("Target: ")); LR_SPRNL(pSvoPinAction->TargetPosition); LR_SPRN(F("Current: ")); LR_SPRNL(pSvoPinAction->StartPosition);	LR_SPRN(F("Time: ")); LR_SPRNL(pSvoPinAction->ServoTime);
		break;
	}
	default:
		break;
	}
}

bool PWMOutputManager::ExecuteAspectForPin(uint8_t &PinID, const uint8_t &Direction)
{
	//LR_SPRN(F("Ap:"));	LR_SPRNL(PinID);
	
	uint8_t basepin = GetAspectBasePin(PinID);
	if (lastLoadedAspectPin != PinID) //TODO check if this type of validation works as expected
	{		
		uint16_t addrPos = SV_ACCESSORY_DECODER_ASPECTS_DATA_START + (sizeof(struct__ConfigurationPWMPin) * PinID);
		if (NVRAM::EEPROMget(addrPos, reinterpret_cast<byte*>(lastLoadedConfig), sizeof(struct__ConfigurationPWMPin)) == 0)
		{
			//LR_SPRNL(F("EGF"));
		}
		lastLoadedAspectPin = PinID;
	}
	if (Direction)
	{
		ProcessAspectChange(lastLoadedConfig->pinAspectsRed, basepin);
	}
	else
	{
		ProcessAspectChange(lastLoadedConfig->pinAspectsGreen, basepin);
	}
}

void PWMOutputManager::NotifyAccTurnoutOutput(const uint16_t &Addr, const uint8_t &Direction)
{
	for (uint8_t PinID = 0; PinID < MAX_NUMBER_OF_PWM_OUTPUT_PINS; PinID++)
	{
		//LR_SPRN(F("PinA:")); LR_SPRNL(addresses[PinID].dccAddress);
		if (addresses[PinID].dccAddress != 0 && Addr != 0 && addresses[PinID].dccAddress == Addr)
		{
			ExecuteAspectForPin(PinID, Direction);
		}
	}
}

void PWMOutputManager::process()
{
	if (pendingFactoryResetPins != 0)
	{
		/*
		* If there is a factory reset.Clear all NVRAM.
		* per process loop, clear one pin config
		*/
		pendingFactoryResetPins--;
		if (lastLoadedAspectPin != LAST_LOADED_PIN_NILL)
		{
			memset(lastLoadedConfig, 0, sizeof(struct__ConfigurationPWMPin));
			lastLoadedAspectPin = LAST_LOADED_PIN_NILL;
		}
		/*
		* The EEPROM has a finite life. In Arduino, the EEPROM is specified to handle 100 000 I2C_Write/erase cycles for each position.
		* However, reads are unlimited. This means you can read from the EEPROM as many times as you want without compromising its life expectancy.
		*/
		//LR_SPRN(F("Erase config:")); LR_SPRNL(pendingFactoryResetPins);
		NVRAM::EEPROMErase(SV_ACCESSORY_DECODER_ASPECTS_DATA_START + (sizeof(struct__ConfigurationPWMPin) * pendingFactoryResetPins), sizeof(struct__ConfigurationPWMPin));
	}

	if (CurrentPinToUpdate != 0 || (millis() - previousRefreshMillis) >= REFRESH_INTERVAL)
	{
		if (CurrentPinToUpdate == 0)
		{
			previousRefreshMillis = millis();
			pwmOffSkip++;
		}

		//Update one servo per loop, stop when at the end of the list
		if (CurrentPinToUpdate >= MAX_NUMBER_OF_PWM_OUTPUT_PINS)
		{
			UpdateServoThisProcessLoop++;
			CurrentPinToUpdate = 0;
			return;
		}
		ProcessOutputPin(CurrentPinToUpdate);
		CurrentPinToUpdate++;
	}
	//}
}

/*
uint16_t PWMOutputManager::NextAddrToRetrievePos()
{
	if (PinPendingStatusRetrieval >= 32)
	{
		return 0;
	}
	else
	{
		uint8_t usePrevious = addresses[PinPendingStatusRetrieval].usePreviousPin;
		uint16_t addr = addresses[PinPendingStatusRetrieval].dccAddress;

		if(!usePrevious)
		{
			
			if (addr != 0)
			{
				LR_SPRN("req: ");
				LR_SPRNL(addr);
				return addr;
			}
		}
	}
}*/

void PWMOutputManager::ProcessOutputPin(uint8_t i)
{
	//LR_SPRN("pin: "); LR_SPRN(i); LR_SPRNL("Act: "); pinPWMAction[i].Action;
	switch (pinPWMAction[i].Action)
	{
		//DO NOT DECLARE VARIABLE IN SWITCH STATEMENT OR USE BRACKETS
	case PWM_NONE:
	case PWM_IDLE:
		break;
	case PWM_TURN_OFF:
		//LR_SPRN("F: "); //LR_SPRNL(pinAction[i].InptData.value);
		if (ProcessPWMTurnOff(i, pinPWMAction[i], pinPWMAction[i].InptData.value))
		{
			//Once offTime, the action can be cleared??
			//Todo: Check if this is the case --> 8/APR/23 probaly not the case, as this will cause strange behaviour when switching aspect
			// 24/4 doubt if it is a problem
			pinPWMAction[i].Action = PIN_PWM_Action::PWM_IDLE;
		}
		break;
	case PWM_FULL_ON:
		//LR_SPRNL("O");
		if (ProcessPWMFullON(i, pinPWMAction[i], pinPWMAction[i].InptData.value))
		{
			//Once onTime, the action can be cleared ??
			//Todo: Check if this is the case
			//pinAction[i].Action = PIN_PWM_ACTION::PWM_IDLE;
		}
		break;
	case PWM_DIMMED_ON:
		{
			//LR_SPRNL("D");
			uint8_t refVal = pinPWMAction[i].InptData.DimmedData.FadeTime;
			ProcessPWMDimmedON(i, pinPWMAction[i], refVal);
		}
		break;
	case PWM_BLINK:
		//LR_SPRNL("B");
		ProcessPWMBlink(i, pinPWMAction[i]);
		break;
	case PWM_BLINK_INV:
		//LR_SPRNL("I");
		if (pinPWMAction[i].PinStatusBits == PIN_AspectStatus::ASPECT_INITIAL)
		{
			pinPWMAction[i].PinStatusBits = PIN_AspectStatus::ASPECT_TURN_OFF_NO_PWM_OUT;
			pinPWMAction[i].FadeLevel = PWM_FADELEVEL_MAX;
		}
		ProcessPWMBlink(i, pinPWMAction[i]);
		break;
	case PWM_SERVO_POS:
	case PWM_SERVO_POS_ND:
		if (!UpdateServoThisProcessLoop)
		{
			ProcessServo(i, pinPWMAction[i]);
		}
		break;
	case PWM_RANDOM_ON:
		//LR_SPRNL("R");
		ProcessPWMRandomOn(i, pinPWMAction[i]);
		break;
	case PWM_RED_TROUGH_YELLOW:
		if (pinPWMAction[i].PinStatusBits == PIN_AspectStatus::ASPECT_OFF_WAIT)
		{
			//LR_SPRNL("Done");
			UpdateAspectActionToNew(i, PIN_PWM_Action::PWM_NONE);
			pinPWMAction[i].PinStatusBits = PIN_AspectStatus::ASPECT_INITIAL;
			pinPWMAction[i].FadeLevel = 0;
			UpdateAspectActionToNew(i - 2, PIN_PWM_Action::PWM_FULL_ON); //Red Signal index, see case PIN_ASPECT_INSTRUCTION::RedViaYellow: for details
		}
		else
		{
			ProcessPWMBlink(i, pinPWMAction[i]);
		}
		break;
	default:
		//LR_SPRNL("?");
		break;
	}
	//LR_SPRNL("n");
}

uint8_t PWMOutputManager::GetCurrentServoPositionAsUint8(struct__PinI2CServoPinAction* pSvoPinAction)
{
	return pSvoPinAction->CurrentPosition10bits / 4;
}

void PWMOutputManager::SetUint8ToCurrentServoPosition(const uint8_t &currentPos, struct__PinI2CServoPinAction* pSvoPinAction)
{
	pSvoPinAction->CurrentPosition10bits = (unsigned int)(currentPos * 4);
}

void PWMOutputManager::SetFloatToCurrentServoPosition(const float &currentPos, struct__PinI2CServoPinAction* pSvoPinAction)
{
	pSvoPinAction->CurrentPosition10bits = (unsigned int)(currentPos * 4);
}

void PWMOutputManager::UpdateServoCurrentPosition(struct__PinI2CServoPinAction* pSvoPinAction, float& stepValue)
{
	byte CurrentPosition = GetCurrentServoPositionAsUint8(pSvoPinAction);
	int16_t newPos = ((int16_t)pSvoPinAction->CurrentPosition10bits) + stepValue;

	if (newPos == pSvoPinAction->CurrentPosition10bits)
	{
		//LR_SPRN("==");
		if (stepValue < 0)
			newPos = pSvoPinAction->CurrentPosition10bits - 1;
		else
			newPos = pSvoPinAction->CurrentPosition10bits + 1;
	}

	//LR_SPRN("newPos:");	LR_SPRNL(newPos);
	if (newPos < 0)
		newPos = 0;
	else if (newPos > 0x3FF)
		newPos = 0x3FF;

	if (CurrentPosition < pSvoPinAction->TargetPosition)
	{ //current value lower then target value
		pSvoPinAction->CurrentPosition10bits = newPos; //increase
		if (GetCurrentServoPositionAsUint8(pSvoPinAction) > pSvoPinAction->TargetPosition)
			SetUint8ToCurrentServoPosition(pSvoPinAction->TargetPosition, pSvoPinAction); //Not higher then targetPosition
	}
	else
	{ //current value higher then target value
		pSvoPinAction->CurrentPosition10bits = newPos; //decrease
		if (GetCurrentServoPositionAsUint8(pSvoPinAction) < pSvoPinAction->TargetPosition)
			SetUint8ToCurrentServoPosition(pSvoPinAction->TargetPosition, pSvoPinAction); //Not lower then targetPosition
	}
}

void PWMOutputManager::ProcessServo(uint8_t pinIndex, struct__PinI2CPWMPinAction& pinAction)
{
	//Reinterpet data for pin as servo action data
	struct__PinI2CServoPinAction* pSvoPinActionCast = reinterpret_cast<struct__PinI2CServoPinAction*>(&pinAction);

	if (GetCurrentServoPositionAsUint8(pSvoPinActionCast) == pSvoPinActionCast->TargetPosition)
	{
		if (pSvoPinActionCast->Action == PIN_PWM_Action::PWM_SERVO_POS_ND)
		{
			//pSvoPinActionCast->PinStatusBits = PIN_AspectStatus::ASPECT_OFF_WAIT;
			return;
		}
		else if (pSvoPinActionCast->PinStatusBits == PIN_AspectStatus::ASPECT_TURN_ON)
		{
			pSvoPinActionCast->PinStatusBits = PIN_AspectStatus::ASPECT_TURN_OFF;
			pSvoPinActionCast->ServoTime = 0;			
		}
	}

	switch (pSvoPinActionCast->PinStatusBits)
	{
	case PIN_AspectStatus::ASPECT_OFF_WAIT:
		//Nothing to do
		return;
	case PIN_AspectStatus::ASPECT_TURN_OFF:
		//if (pSvoPinActionCast->ServoTime == 0x1F) //Changed to using pwmOffSkip to add more time 
		if (pSvoPinActionCast->ServoTime == 0x02)
		{
			//LR_SPRNL("Discon");
			pSvoPinActionCast->PinStatusBits = PIN_AspectStatus::ASPECT_OFF_WAIT;
			SetPinServoPWM(pinIndex, PWM_DISABLE_SERVO);
			return;
		}
		else
		{
			if (pwmOffSkip == 0)
			{
				pSvoPinActionCast->ServoTime++;
			}
		}
		break;

	case PIN_AspectStatus::ASPECT_INITIAL:
		//LR_SPRNL("InitSvo");
		SetUint8ToCurrentServoPosition(pSvoPinActionCast->TargetPosition, pSvoPinActionCast);
		pSvoPinActionCast->PinStatusBits = PIN_AspectStatus::ASPECT_TURN_ON;
		break;

	case PIN_AspectStatus::ASPECT_TURN_ON:
		//check if pin state A <> B
		if (pSvoPinActionCast->StartPosition == pSvoPinActionCast->TargetPosition)
		{
			SetUint8ToCurrentServoPosition(pSvoPinActionCast->TargetPosition, pSvoPinActionCast);
		}
		else
		{
			//Calculate Delta
			int16_t delta = pSvoPinActionCast->TargetPosition - pSvoPinActionCast->StartPosition;
			float stepValue = (float)delta; //No time is configured
			if (pSvoPinActionCast->ServoTime != 0)
			{ //Check if time is configured
				stepValue = (float)delta / (pSvoPinActionCast->ServoTime * ServoDelayFactor); //Yes, devide
			}
			//LR_SPRN("From:"); LR_SPRNL(SvoPinActionCast->StartPosition); LR_SPRN("To:"); LR_SPRNL(SvoPinActionCast->TargetPosition); LR_SPRN("Delta:" ); LR_SPRNL(delta); LR_SPRN("stepvalue:"); LR_SPRNL(stepValue); LR_SPRN("stepvalueA:"); LR_SPRNL(SvoPinActionCast->CurrentPosition10bits);
			UpdateServoCurrentPosition(pSvoPinActionCast, stepValue);
		}
		break;
	default:
		break;
	}
	SetPinServoPWM(pinIndex, pSvoPinActionCast->CurrentPosition10bits);
}

void PWMOutputManager::ProcessPWMRandomOn(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin)
{
	uint8_t fadetime = 0;
	switch (pin.PinStatusBits)
	{
	case PIN_AspectStatus::ASPECT_INITIAL:
	case PIN_AspectStatus::ASPECT_TURN_ON:
		if (ProcessPWMFullON(pinIndex, pin, fadetime))
		{
			pin.PinStatusBits = PIN_AspectStatus::ASPECT_ON_WAIT;
			pin.InptData.value = random(0, 0x1F);
		}
		return;
	case PIN_AspectStatus::ASPECT_ON_WAIT:
		fadetime = pin.InptData.value;
		if (ProcessPWMSleep(pinIndex, pin, fadetime))
		{
			pin.PinStatusBits = PIN_AspectStatus::ASPECT_TURN_OFF;
			pin.FadeLevel = PWM_FADELEVEL_MAX;
		}
		return;
	case PIN_AspectStatus::ASPECT_TURN_OFF:
		if (ProcessPWMTurnOff(pinIndex, pin, fadetime))
		{
			pin.PinStatusBits = PIN_AspectStatus::ASPECT_OFF_WAIT;
			pin.FadeLevel = PWM_FADELEVEL_MAX;
			pin.InptData.value = random(0, 0xF);
		}
		return;
	case PIN_AspectStatus::ASPECT_OFF_WAIT:
		fadetime = pin.InptData.value;
		if (ProcessPWMSleep(pinIndex, pin, fadetime))
		{
			pin.PinStatusBits = PIN_AspectStatus::ASPECT_TURN_ON;
		}
		return;
	default:
		pin.PinStatusBits = PIN_AspectStatus::ASPECT_TURN_ON;
		break;
	}
}

void PWMOutputManager::ProcessPWMBlink(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin)
{
	if (pin.InptData.value == 0)
		//Pin has no time configuration
		pin.InptData.value = 0xFF;

	uint8_t fadetime = pin.InptData.BlinkData.FadeTime;	
	switch (pin.PinStatusBits)
	{
	case PIN_AspectStatus::ASPECT_TURN_ON:
	case PIN_AspectStatus::ASPECT_INITIAL:
		if (ProcessPWMFullON(pinIndex, pin, fadetime))
		{
			pin.PinStatusBits = PIN_AspectStatus::ASPECT_ON_WAIT;
		}
		return;
	case PIN_AspectStatus::ASPECT_ON_WAIT:
		fadetime = pin.InptData.BlinkData.HoldTime;
		if (ProcessPWMSleep(pinIndex, pin, fadetime))
		{
			pin.PinStatusBits = PIN_AspectStatus::ASPECT_TURN_OFF;
			pin.FadeLevel = PWM_FADELEVEL_MAX;
		}
		return;
	case PIN_AspectStatus::ASPECT_TURN_OFF:
		if (ProcessPWMTurnOff(pinIndex, pin, fadetime))
		{
			pin.PinStatusBits = PIN_AspectStatus::ASPECT_OFF_WAIT;
			pin.FadeLevel = PWM_FADELEVEL_MAX;
		}
		return;
		//Aspect is used in Invert_blink for firts loop only
	case PIN_AspectStatus::ASPECT_TURN_OFF_NO_PWM_OUT:
		if (ProcessPWMSleep(pinIndex, pin, fadetime))
		{
			pin.PinStatusBits = PIN_AspectStatus::ASPECT_OFF_WAIT;
			pin.FadeLevel = PWM_FADELEVEL_MAX;
		}
		break;
	case PIN_AspectStatus::ASPECT_OFF_WAIT:
		fadetime = pin.InptData.BlinkData.HoldTime;
		if (ProcessPWMSleep(pinIndex, pin, fadetime))
		{
			pin.PinStatusBits = PIN_AspectStatus::ASPECT_TURN_ON;
		}
		return;
	default:
		pin.PinStatusBits = PIN_AspectStatus::ASPECT_TURN_ON;
		break;
	}
}

bool PWMOutputManager::ProcessPWMSleep(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin, uint8_t& time)
{
	bool ret = false; //Return if Processing is completed
	if (pin.FadeLevel != 0)
	{
		uint16_t fadestep = CalculateFadeStepValue(time);
		if (pin.FadeLevel <= fadestep)
		{
			ret = true;
			pin.FadeLevel = 0;
		}
		else
		{
			pin.FadeLevel -= fadestep;
		}
	}
	else
	{
		ret = true;
	}
	return ret;
}

bool PWMOutputManager::ProcessPWMTurnOff(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin, uint8_t &time)
{
	bool ret = false; //Return if Processing is completed
	if (pin.FadeLevel != 0)
	{
		uint16_t fadestep = CalculateFadeStepValue(time);
		if (pin.FadeLevel <= fadestep)
		{
			ret = true;
			pin.FadeLevel = 0;
		}
		else
		{
			pin.FadeLevel -= fadestep;
		}
		SetPinLedPWM(pinIndex, pin.FadeLevel);		
		//LR_SPRN("Pin: "); LR_SPRN(pinIndex); LR_SPRN(" FadeStep: "); LR_SPRN(fadestep); LR_SPRN(" ActDataStep: "); LR_SPRNL(pin.FadeLevel);
	}
	else
	{
		ret = true;
	}
	return ret;
}

bool PWMOutputManager::ProcessPWMDimmedOff(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin, uint8_t& time)
{
	bool ret = false; //Return if Processing is completed
	if (pin.FadeLevel != 0)
	{
		uint16_t fadestep = (CalculateFadeStepValue(time) / 15) * pin.InptData.DimmedData.DimmedValue;
		if (pin.FadeLevel <= fadestep)
		{
			ret = true;
			pin.FadeLevel = 0;
		}
		else
		{
			pin.FadeLevel -= fadestep;
		}
		SetPinLedPWM(pinIndex, pin.FadeLevel);
	}
	else
	{
		ret = true;
	}
	return ret;
}


bool PWMOutputManager::ProcessPWMDimmedON(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin, uint8_t& time)
{
	uint16_t L_MAXFADELEVEL = (PWM_FADELEVEL_MAX / 15) * pin.InptData.DimmedData.DimmedValue;
	bool ret = false; //Return if Processing is completed
	if (pin.FadeLevel != L_MAXFADELEVEL)
	{
		uint16_t fadestep = (CalculateFadeStepValue(time) / 15) * pin.InptData.DimmedData.DimmedValue;
		if (pin.FadeLevel + fadestep >= L_MAXFADELEVEL)
		{
			ret = true;
			pin.FadeLevel = L_MAXFADELEVEL;
		}
		else
		{
			pin.FadeLevel += fadestep;
		}
		SetPinLedPWM(pinIndex, pin.FadeLevel);
		//LR_SPRN("Pin: "); LR_SPRN(pinIndex); LR_SPRN(" MaxLevel: "); LR_SPRN(L_MAXFADELEVEL); LR_SPRN(" FadeStep: "); LR_SPRN(fadestep); LR_SPRN(" ActDataStep: "); LR_SPRNL(pin.FadeLevel);
	}
	else
	{
		ret = true;
	}
	return ret;
}

bool PWMOutputManager::ProcessPWMFullON(uint8_t pinIndex, struct__PinI2CPWMPinAction &pin, uint8_t &time)
{
	bool ret = false; //Return if Processing is completed
	if (pin.FadeLevel != PWM_FADELEVEL_MAX)
	{
		uint16_t fadestep = CalculateFadeStepValue(time);
		if (pin.FadeLevel + fadestep >= PWM_FADELEVEL_MAX)
		{
			ret = true;
			pin.FadeLevel = PWM_FADELEVEL_MAX;
		}
		else
		{
			pin.FadeLevel += fadestep;
		}
		SetPinLedPWM(pinIndex, pin.FadeLevel);
		//LR_SPRN("FadeStep: "); LR_SPRN(fadestep); LR_SPRN(" ActDataStep:"); LR_SPRN(pin.FadeLevel); LR_SPRN(" ret:"); LR_SPRNL(ret);

	}
	else
	{
		ret = true;
	}
	return ret;
}

uint16_t PWMOutputManager::CalculateFadeStepValue(uint8_t &time)
{
	if (time == 0)
		return PWM_FADELEVEL_MAX;

	uint16_t fadeStep = PWMFadeTimeCalculationBase / time;
	if (fadeStep < 1)
		fadeStep = 1;

	return fadeStep;
}

void PWMOutputManager::SetPinServoPWM(const uint8_t &PinIndex, const uint16_t &PWM_value)
{
	uint16_t PWMvalueL = 0;
	if (PWM_value != PWM_DISABLE_SERVO)
		PWMvalueL = SERVO_PWM_MIN + (((double)PWM_value) * 0.75);

	uint8_t idx_board = PinIndex / PWM_PINS_PER_BOARD;
	uint8_t idx_boardPin = PinIndex - (idx_board * PWM_PINS_PER_BOARD);

	if (bit_is_set(BitBoardsOnline, 7 - idx_board) != 0)
	{
		I2C_SetPinPWMValue((idx_board + DeviceAddressStart), idx_boardPin, 0, PWMvalueL);
	}
}

void PWMOutputManager::SetPinLedPWM(uint8_t PinIndex, uint16_t PWMvalue)
{
	//LR_SPRN(F("Pin:")); LR_SPRN(PinIndex); LR_SPRN(F(" val:")); LR_SPRN(PWMvalue); LR_SPRN(F(" Inv:")); LR_SPRNL(addresses[PinIndex].InvertPowerForOutputPin);

	uint8_t idx_board = PinIndex / PWM_PINS_PER_BOARD;
	uint8_t idx_boardPin = PinIndex - (idx_board * PWM_PINS_PER_BOARD);

	if (bit_is_set(BitBoardsOnline, 7 - idx_board) != 0)
	{
		if (addresses[PinIndex].InvertPowerForOutputPin == 0)
		{
			switch (PWMvalue)
			{
			case 0:
				I2C_SetPinPWMValue((idx_board + DeviceAddressStart), idx_boardPin, 0, PWM_FADELEVEL_FULL_NOPWM);
				break;
			case PWM_FADELEVEL_MAX:
				I2C_SetPinPWMValue((idx_board + DeviceAddressStart), idx_boardPin, PWM_FADELEVEL_FULL_NOPWM, 0);
				break;
			default:
				I2C_SetPinPWMValue((idx_board + DeviceAddressStart), idx_boardPin, 0, PWMvalue);
				break;
			}
		}
		else 
		{
			switch (PWMvalue)
			{
			case 0:
				I2C_SetPinPWMValue((idx_board + DeviceAddressStart), idx_boardPin, PWM_FADELEVEL_FULL_NOPWM, 0);
				break;
			case PWM_FADELEVEL_MAX:
				I2C_SetPinPWMValue((idx_board + DeviceAddressStart), idx_boardPin, 0, PWM_FADELEVEL_FULL_NOPWM);
				break;
			default:
				I2C_SetPinPWMValue((idx_board + DeviceAddressStart), idx_boardPin, 0, PWM_FADELEVEL_MAX - PWMvalue);
				break;
			}
		}
	}
}

bool PWMOutputManager::I2C_SetPinPWMValue(const uint8_t& addr, const uint8_t &PinIndex, const uint16_t &onTime, const uint16_t &offTime)
{
	uint8_t buffer[5];
	buffer[0] = PCA9685_LED0 + (4 * PinIndex);
	buffer[1] = onTime;
	buffer[2] = onTime >> 8;
	buffer[3] = offTime;
	buffer[4] = offTime >> 8;

	return I2C_Write(addr, buffer, 5);
}

bool PWMOutputManager::I2C_WriteRegister(const uint8_t& addr, const uint8_t& regaddr, const uint8_t& value)
{
	uint8_t data[2] = { regaddr, value };
	return I2C_Write(addr, data, 2);
	delay(25);
}

void PWMOutputManager::I2C_DeviceRestart(const uint8_t& addr)
{
	I2C_WriteRegister(addr, PCA9685_MODE1, PCA9685_MODE1_RESTART);
}

void PWMOutputManager::I2C_SetPWMFreq(const uint8_t& addr)
{
	uint8_t current_value = I2C_ReadRegister(addr, PCA9685_MODE1);
	uint8_t new_value = (current_value & ~PCA9685_MODE1_RESTART) | PCA9685_MODE1_SLEEP; 
	I2C_WriteRegister(addr, PCA9685_MODE1, new_value);
	I2C_WriteRegister(addr, PCA9685_PRESCALE, PCA9685_PRESCALE_VALUE); // set the prescaler
	I2C_WriteRegister(addr, PCA9685_MODE1, current_value);	
	I2C_WriteRegister(addr, PCA9685_MODE1, current_value | PCA9685_MODE1_RESTART | PCA9685_MODE1_AUTOINCREASE);
}

uint8_t PWMOutputManager::I2C_ReadRegister(const uint8_t &addr, const uint8_t &regaddr)
{
	uint8_t buffer[1] = { regaddr };
	if (!I2C_Write(addr, buffer, 1))
	{
		return 0;
	}

	if (Wire.requestFrom((int)addr, (int)1) == 1)
	{		
		buffer[0] = Wire.read();
		return buffer[0];
	}

	return 0;
}