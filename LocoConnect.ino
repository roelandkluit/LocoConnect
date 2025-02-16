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
#include "NVRAM_AT24C32.h"
#include "CompileConfig.h"
#include "NVRAM.h"
#include "ButtonManager.h"
#include "CustDefines.h"
#include "version.h"
#include "BoardLeds.h"
#include <avr/wdt.h>
#include "LoconetExt.h"

#ifdef REQUIRES_I2C
    #include <Wire.h>
    #include "I2CBase.h"
#endif 

#if defined(NATIVE_S88_ENABLED) || defined (LOCAL_I2C_S88_ENABLED)
    #include "S88Interface.h"
#endif

#ifdef LOCAL_S88_ENABLED
    #include "LocalOccupation.h"
#endif

#if defined(PCA9685_I2C_ADDR_START)
    #include "PWMOutputManager.h"
#endif

#if defined(VL53L0XVersion)
    #include "S88VL53L0X.h"
#endif

#ifdef LOCAL_S88_ENABLED
    byte LocalOccupationPins[] = { AUXPIN1, AUXPIN2, AUXPIN3, AUXPIN4 };
    LocalOccupation S88_Local(LocalOccupationPins, sizeof(LocalOccupationPins));
#endif

#ifdef NATIVE_S88_ENABLED
    S88Interface S88_Native(S88_DATA1, S88_CLOCK, S88_LOAD, S88_RESET);// , S88_DATA2);
#endif

#ifdef LOCAL_I2C_S88_ENABLED
    S88Interface S88_I2C(PCF8574AT_I2C_ADDR);
    uint16_t S88_I2C_StartIndex = 0;
#endif

#ifdef PCA9685_I2C_ADDR_START
    PWMOutputManager pca9685I2C;
    struct__Servo_Position_Override ServoPositioningSV;
#endif

#ifdef  VL53L0XVersion
    S88VL53L0X vlx;
#endif

#ifdef PROGRAM_BUTTON_ENABLED
    ButtonManager ButtonProgram(BUTTON_PROGRAM, true);
#endif //  PROGRAM_BUTTON_ENABLED

ButtonManager ButtonStopGo(BUTTON_STOPGO, true);

lnMsg* LnPacket;
LocoNetSystemVariableClass sv;
uint16_t S88_Native_StartIndex_Chain1 = 0;
uint16_t S88_Local_StartIndex = 0;
uint16_t S88_VL53L0X_StartIndex = 0;
uint8_t S88_Local_EnableCount = 0; 
uint8_t S88_Native_ModuleCount_Chain1 = 2;
uint16_t LastReportedSensorAddress = 0; //Previsouly used to check for collition, 
                                          //changed to hardware check if line is free and if submission was succesfull
                                          //Disabled for now

uint8_t iLoopWaitCounter = 0;
LocoRunStatus LoconetRunStatus = LocoRunStatus::LoconetInitialLedOffState;

struct lByteBools
{
    unsigned deferredProcessingNeeded : 1;
    unsigned isJustBooted : 1;
    unsigned bResend_Last_Loconet_Packet : 1;
    unsigned bAT24CXXisPresent : 1;
    unsigned reserved : 4;
};
lByteBools bBools;

unsigned long LoconetBusyLastMillis = 0;
uint8_t LoconetBusyWaitTime = 0;

void OnButtonGoStopPress(const bool &LongPress)
{
    LN_STATUS ret;
    if (LongPress == false)
    {
        //BTN: STOP
        ret = LocoNet.reportPower(0);
        if(LoconetRunStatus == LocoRunStatus::LoconetGo)
            LoconetRunStatus = LoconetStoppedLocalLedOffState;
    }
    else
    {
        //BTN: GO
        ret = LocoNet.reportPower(1);
    }
    if (ret == LN_DONE)
    {
        BoardLeds::SetSysLedON(25);
    }
}

void notifySensor(uint16_t Address, uint8_t State)
{
    if (Address == (LastReportedSensorAddress & 0x7FFF))
    {
        //LR_SPRN(F("Rsens")); LR_SPRNL(Address);
        
        LastReportedSensorAddress = 0;
        SetLoconetBusyWait();
        LoconetBusyWaitTime = 2;
    }
    else
    if (LastReportedSensorAddress != 0)
    {
        bBools.bResend_Last_Loconet_Packet = true;
        SetLoconetBusyWait();
        //LR_SPRNL(F(".!M"));
    }
    else
    {
        //LR_SPRNL(F(".OD"));        
        SetLoconetBusyWait();
    }
}

LN_STATUS reportSensorState(uint16_t Address, uint8_t State)
{
    if (ProcessLoconetPackets())
    {
        return LN_STATUS::LN_CD_BACKOFF;
    }
    if (!LocoNetClassExt::uartStateIdle())
    {
        //LR_SPRNL(F("NOTREADY"));
        return LN_STATUS::LN_CD_BACKOFF;
    }    
    else if (LastReportedSensorAddress != 0 && LastReportedSensorAddress != Address)
    {
        return LN_STATUS::LN_CD_BACKOFF;
    }
    else if (LoconetBusyLastMillis != 0)
    {
        return LN_STATUS::LN_CD_BACKOFF;
    }

    LN_STATUS result = LocoNet.reportSensor(Address, State);
    if (result == LN_STATUS::LN_DONE)
    {
        //Encode State in first bit, needed in case of retransmit
        LastReportedSensorAddress = (Address & 0x7FFF) + (uint16_t(State) << 15);
    }
    return result;
}

#ifdef PROGRAM_BUTTON_ENABLED
    void OnButtonProgramPress(const bool &LongPress)
    {
        if(LoconetRunStatus < LocoRunStatus::LoconetProg_StartAddressState && LongPress)
        //if (ProgrammingMode == ProgrammingModes::PROGAMMODE_NONE && LongPress)
        {
            //LR_SPRNL(F("Programing started, provide start adress"));
            //ProgrammingMode = ProgrammingModes::PROGRAMMODE_STARTADDRESS;
            LoconetRunStatus = LocoRunStatus::LoconetProg_StartAddressState;

        }
        else if (LoconetRunStatus >= LocoRunStatus::LoconetProg_StartAddressState && LongPress)
        //else if (ProgrammingMode != ProgrammingModes::PROGAMMODE_NONE && LongPress)
        {
            //LR_SPRNL(F("Programing canceled"));
            //ProgrammingMode = ProgrammingModes::PROGAMMODE_NONE;
            LoconetRunStatus = LocoRunStatus::LoconetInitialLedOffState;
        }
    }
#endif

void notifyPower(uint8_t State)
{
    //LR_SPRN(F("PWR:")); LR_SPRN(State); LR_SPRN(F(", PLS:")); LR_SPRNL(LoconetRunStatus);
    if (State)
    {
        if (LoconetRunStatus != LocoRunStatus::LoconetGo)
        {
            SetRunningStatus(true);
            LoconetRunStatus = LocoRunStatus::LoconetGo;
        }
    }
    else
    {
        if (!(LoconetRunStatus == LocoRunStatus::LoconetStopped || LoconetRunStatus == LocoRunStatus::LoconetStoppedLocalLedOffState || LoconetRunStatus == LocoRunStatus::LoconetStoppedLocalLedOnState))
        {
            SetRunningStatus(false);
        }
    }
}

bool ProcessLoconetPackets()
{
    bool returnValue = false;
    // Check for any received LocoNet packets

    LnPacket = LocoNet.receive();
    if (LnPacket)
    {
        returnValue = true;
        //Be nice to other devices and give them bus time
        SetLoconetBusyWait();

        //Process LoconetPacket
        if (LocoNet.processSwitchSensorMessage(LnPacket))
        {
            //Is processed in LocoNet class, no futher action
            return;
        }
        else if (LnPacket->sz.command == OPC_PEER_XFER) //LNCV & LNSV Messages
        {
            //Process LNSV packets
            SV_STATUS svStatus = sv.processMessage(LnPacket);
            if (!bBools.deferredProcessingNeeded)
            {
                bBools.deferredProcessingNeeded = (svStatus == SV_DEFERRED_PROCESSING_NEEDED);
            }
        }
    }

    //Process pending SV data if needed
    if (bBools.deferredProcessingNeeded && LoconetBusyLastMillis == 0)
    {
        bBools.deferredProcessingNeeded = (sv.doDeferredProcessing() != SV_OK);
    }
    return returnValue;
}

/*
void notifySwitchState(uint16_t Address, uint8_t Output, uint8_t Direction)
{
    lastNotifySwitchAddr = Address;
    lastNotifySwitchDirection = Direction;
}

// Used to retrieve current turnout position, currently not used
void notifyLongAck(uint8_t d1, uint8_t d2)
{
    #ifdef PCA9685_I2C_ADDR_START    
        if (ServoStartAdress != 0 && lastNotifySwitchAddr != 0)
        {
            if (ServoStartAdress <= lastNotifySwitchAddr && lastNotifySwitchAddr < ServoStartAdress + ServoMgrI2C.GetPinCount())
            {
                //<0, 0, C, T - A10, A9, A8, A7> Report / status bits and 4 MS adr bits
                uint8_t Accepted = (d2 & 0x40) >> 6; //Mischien 0x80 ??? met een >> 5
                //LR_SPRN(lastNotifySwitchAddr - ServoStartAdress);
                //LR_SPRN('=');
                //LR_SPRN(lastNotifySwitchDirection ? "Closed" : "Thrown");
                //LR_SPRN("==");
                //LR_SPRN(Accepted ? "True" : "False");
                //LR_SPRN("==");
                //LR_SPRNL((Accepted && !lastNotifySwitchDirection) ? F("Closed") : F("Thrown"));
                bool dirCurved = Accepted && !lastNotifySwitchDirection;
                ServoMgrI2C.SetServoPosition(lastNotifySwitchAddr - ServoStartAdress, dirCurved, false);
                lastNotifySwitchAddr = 0;
            }
        }
    #endif
}
*/

void notifySwitchRequest(uint16_t Address, uint8_t Output, uint8_t Direction)
{
    //We care differently about switch messages when in programming mode
    #ifdef PROGRAM_BUTTON_ENABLED
        if(LoconetRunStatus >= LocoRunStatus::LoconetProg_StartAddressState)
        //if (ProgrammingMode != ProgrammingModes::PROGAMMODE_NONE)
        {
            if (Output)
            {
                //if (ProgrammingMode == ProgrammingModes::PROGRAMMODE_STARTADDRESS)
                if (LoconetRunStatus == LocoRunStatus::LoconetProg_StartAddressState)
                {
                    //Configured start adress to: %Address%
                    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_S88_START_ADDRES, Address);
                    NVRAM::setSV(SV_ADDR_NODE_ID_L, Address & 0xFF);
                    NVRAM::setSV(SV_ADDR_NODE_ID_H, Address >> 8 );
                    //ProgrammingMode = ProgrammingModes::PROGRAMMODE_MODULECOUNT;
                    LoconetRunStatus = LocoRunStatus::LoconetProg_StartModCountState;
                }
                //else if (ProgrammingMode == ProgrammingModes::PROGRAMMODE_MODULECOUNT)
                else if (LoconetRunStatus == LocoRunStatus::LoconetProg_StartModCountState)
                {
                    #ifdef NATIVE_S88_ENABLED
                        if (Address > S88_Native.GetMaxModuleCount())
                        {
                            Address = S88_Native.GetMaxModuleCount();
                        }
                    #endif
                    //Configured module count to: %Address%
                    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_S88_ADDRES_COUNT, Address);
                    //ProgrammingMode = ProgrammingModes::PROGAMMODE_NONE;
                    LoconetRunStatus = LocoRunStatus::LoconetInitialLedOffState;
                    GetConfiguredValues();
                    //SetRunningStatus(LoconetRunStatus == LocoRunStatus::LoconetGo);
                }
            }
        }
        else
    #endif
    {
        //LR_SPRN(F("TO:")); LR_SPRNL(Address);
        #ifdef PCA9685_I2C_ADDR_START
                pca9685I2C.NotifyAccTurnoutOutput(Address, Direction);
        #endif
    }
}

void OnReportSensor(const uint16_t &report_index, const uint8_t &Status, bool& success)
{
    if (reportSensorState(report_index, Status) != LN_DONE)
    {
        success = false;
        return;
    }

    BoardLeds::SetSysLedON(5);
    success = true;
}

#ifdef  LOCAL_I2C_S88_ENABLED
void OnReportI2CS88(const uint8_t& index, const uint8_t& Status, bool& success)
{
    if (S88_I2C_StartIndex != 0)
    {
        OnReportSensor(index + S88_I2C_StartIndex, Status, success);
    }
}
#endif

#ifdef VL53L0XVersion
void OnReportVL53LS88(const uint8_t& index, const uint8_t& Status, bool& success)
{
    if (S88_VL53L0X_StartIndex != 0)
    {
        OnReportSensor(index + S88_VL53L0X_StartIndex, Status, success);
    }
}
#endif

void OnReportLocalS88(const uint8_t& index, const uint8_t& Status, bool& success)
{
    if (S88_Local_StartIndex != 0)
    {
        OnReportSensor(index + S88_Local_StartIndex, Status, success);
    }
}

void OnReportS88(const uint8_t &index, const uint8_t &Status, bool& success)
{
    if (S88_Native_StartIndex_Chain1 != 0)
    {
        OnReportSensor(index + S88_Native_StartIndex_Chain1, Status, success);
    }
}

void GetConfiguredValues()
{
    if (LoconetRunStatus >= LocoRunStatus::LoconetProg_StartAddressState)
    {
        //Do not update config if programming mode is still active
        return;
    }
    #ifdef PCA9685_I2C_ADDR_START
        pca9685I2C.CVValueHasBeenChanged();
    #endif

    if (NVRAM::readLNCV(CV_ACCESSORY_DECODER_REPORT_BEHAVIOUR) != S88ReportingOptions::S88ReportOnlyWhenOnGo)
    {
        //When report always is enabled, we should not wait for a GO loconet signal, but start processing after the inital boot timeout
        bBools.isJustBooted = false;
    }

    S88Base::S88ReportingBehavour = NVRAM::readLNCV(CV_ACCESSORY_DECODER_REPORT_BEHAVIOUR);

    //Native S88 Chain
    #ifdef NATIVE_S88_ENABLED
        uint16_t local__S88_Native_StartIndex_Chain1 = NVRAM::readLNCV(CV_ACCESSORY_DECODER_S88_START_ADDRES);
        uint16_t local__S88_Native_ModuleCount_Chain1 = NVRAM::readLNCV(CV_ACCESSORY_DECODER_S88_ADDRES_COUNT);

        if (S88_Native_ModuleCount_Chain1 == 0 || local__S88_Native_StartIndex_Chain1 == 0 || local__S88_Native_StartIndex_Chain1 != S88_Native_StartIndex_Chain1 || local__S88_Native_ModuleCount_Chain1 != S88_Native_ModuleCount_Chain1)
        {   
            //Only update if values have changed
            S88_Native_StartIndex_Chain1 = local__S88_Native_StartIndex_Chain1;
            S88_Native_ModuleCount_Chain1 = local__S88_Native_ModuleCount_Chain1;

            if (S88_Native_ModuleCount_Chain1 > S88_Native.GetMaxModuleCount())
            {
                S88_Native_ModuleCount_Chain1 = S88_Native.GetMaxModuleCount();
            }
            S88_Native.SetS88moduleCount(S88_Native_ModuleCount_Chain1);
            S88_Native.ReportAll();
        }
    #endif

    //Local connected S88 Chain (Aux pins)
    #ifdef LOCAL_S88_ENABLED
        uint16_t local__S88_Local_StartIndex = NVRAM::readLNCV(CV_ACCESSORY_DECODER_LOCAL_START_ADDRES);    
        uint16_t local__S88_Local_EnableCount = NVRAM::readLNCV(CV_ACCESSORY_DECODER_LOCAL_READ_COUNT);
    
        if (local__S88_Local_EnableCount > sizeof(LocalOccupationPins))//Limit pins to maxium AUX pins
        {
            local__S88_Local_EnableCount = sizeof(LocalOccupationPins);
            NVRAM::writeLNCV(CV_ACCESSORY_DECODER_LOCAL_READ_COUNT, local__S88_Local_EnableCount);
        }

        if (local__S88_Local_StartIndex != S88_Local_StartIndex || local__S88_Local_EnableCount != S88_Local_EnableCount)
        {
            //Only update if values have changed
            S88_Local_StartIndex = local__S88_Local_StartIndex;
            S88_Local_EnableCount = local__S88_Local_EnableCount;

            if (S88_Local_StartIndex > 0)
            {
                S88_Local.SetEnabledPins(S88_Local_EnableCount);
                S88_Local.ReportAll();
            }
            else
            {
                S88_Local_EnableCount = 0;
                S88_Local.SetEnabledPins(0);
            }
        }
    #endif

    #ifdef  VL53L0XVersion
        uint16_t local__S88_VL53L0X_StartIndex = NVRAM::readLNCV(CV_ACCESSORY_VL53L0X_START_ADDRES);
        S88_VL53L0X_StartIndex = local__S88_VL53L0X_StartIndex;
        Serial.println(S88_VL53L0X_StartIndex);
        vlx.LoadPinValuesFromNVRAM();
    #endif //  VL53L0XVersion

    #ifdef LOCAL_I2C_S88_ENABLED
        //S88 I2C Chain
        uint16_t local__S88_I2C_StartIndex = NVRAM::readLNCV(CV_ACCESSORY_DECODER_S88_I2C_START_ADDRES);
        uint16_t local__S88_I2C_ModuleCountInfo = NVRAM::readLNCV(CV_ACCESSORY_DECODER_S88_I2C_ADDRES_COUNT);
        uint8_t local__S88_I2C_ModuleCount = (local__S88_I2C_ModuleCountInfo & 0xFF);
        uint8_t local_S88_USE_TIC2_Modules = local__S88_I2C_ModuleCountInfo >> 8;

        if (local_S88_USE_TIC2_Modules == 0)
        {
            S88_I2C.ChangeI2C_StartAddr(PCF8574AT_I2C_ADDR);
            LR_SPRNL(F("AT mod"));
        }
        else
        {
            S88_I2C.ChangeI2C_StartAddr(PCF8574T_I2C_ADDR);
            LR_SPRNL(F("T mod"));
        }

        if (local__S88_I2C_StartIndex != S88_I2C_StartIndex || local__S88_I2C_ModuleCount != S88_I2C.GetS88moduleCount())
        {
            //Only update if values have changed
            S88_I2C_StartIndex = local__S88_I2C_StartIndex;
            
            if (local__S88_I2C_ModuleCount > S88_I2C.GetMaxModuleCount())
            {
                local__S88_I2C_ModuleCount = S88_I2C.GetMaxModuleCount();
            }
            S88_I2C.SetS88moduleCount(local__S88_I2C_ModuleCount);
            S88_I2C.ReportAll();
        }
    #endif

    //Stop Go Button
    ButtonStopGo.SetDefaultStateIsHigh(!(bool)(NVRAM::readLNCV(CV_ACCESSORY_DECODER_STOPGO_DEFAULTNC)));
}

void notifySVChange(const uint16_t &SV, const uint8_t &Value)
{
    GetConfiguredValues();
}

void MakeRandomNumber(uint16_t Addr, bool Force)
{
    uint16_t Val1 = sv.readSVStorage(Addr + 1); //Use Read and write these SV's as these values are offset -2 by the Loconet Class
    uint16_t Val2 = sv.readSVStorage(Addr);

    if (Force || (Val1 == 0x0 && Val2 == 0x0) || (Val1 == 0xFF && Val2 == 0xFF))
    {
        Val1 = random(1, 0xFF);
        Val2 = random(1, 0xFF);
        sv.writeSVStorage(Addr + 1, Val1);
        sv.writeSVStorage(Addr, Val2);
        NVRAM::setSV(SV_ADDR_HW_VER_MEM, ACTIVE_HARDWARE_VERSION);
    }
}

void MakeRandomSerialNumber(bool Force = false)
{
    MakeRandomNumber(SV_ADDR_SERIAL_NUMBER_L, Force);
    MakeRandomNumber(SV_ADDR_NODE_ID_L, Force);
}

/*bool checkVersion()
{
    byte val = NVRAM::getSV(SV_ADDR_HW_VER_MEM);
    switch (val)
    {
        case ACTIVE_HARDWARE_VERSION:
            return true;
        case 0:
        case 0xFF:
            MakeRandomSerialNumber(true);
            return true;
        default:
            return false;
    }   
}*/

void ConfigDefaults()
{
    MakeRandomSerialNumber(true);
    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_CONFIGURED, VAL_ACCESSORY_DECODER_CONFIGURED_CHECK);
    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_S88_START_ADDRES, 1);
    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_S88_ADDRES_COUNT, 4);
    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_S88_CH2_START_ADDRES, 0);
    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_S88_CH2_ADDRES_COUNT, 0);
    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_S88_I2C_START_ADDRES, 0);
    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_S88_I2C_ADDRES_COUNT, 0);
    NVRAM::writeLNCV(CV_ACCESSORY_VL53L0X_START_ADDRES, 0);

    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_STOPGO_DEFAULTNC, 0);
    #ifdef LOCAL_S88_ENABLED
        NVRAM::writeLNCV(CV_ACCESSORY_DECODER_LOCAL_READ_COUNT, sizeof(LocalOccupationPins));
    #endif
    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_LOCAL_START_ADDRES, 0);
    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_REPORT_BEHAVIOUR, 1);

    for (uint8_t i = SV_MEM_STRING_NAME; i <= SV_MEM_STRING_END; i++)
    {       
        uint8_t pos = i - SV_MEM_STRING_NAME;
        if (pos < sizeof(DEFAULT_DEVICE_NAME_STRING))
        {
            char p = pgm_read_byte_near(DEFAULT_DEVICE_NAME_STRING + pos);
            NVRAM::setSV(i, p);
        }
        else
        {
            NVRAM::setSV(i, 0);
        }
    }
    #ifdef PCA9685_I2C_ADDR_START
        pca9685I2C.SetFactoryDefaults();
    #endif    
}

void setup()
{
    randomSeed(analogRead(A0) + (analogRead(A1) * 2) + (analogRead(A2) * 3) + (analogRead(A3) * 4) + (analogRead(A4) * 5) + (analogRead(A5)*6) + (analogRead(A6)*7));    

    delay(250); //Needed for I2C

    #ifdef WITH_SERIAL
        Serial.begin(9600);
    #endif

    while (!NVRAM::InternalEEPROMReady())
    {
        delay(500);
    }

    wdt_enable(WDTO_8S);

    if (NVRAM::readLNCV(CV_ACCESSORY_DECODER_CONFIGURED) != VAL_ACCESSORY_DECODER_CONFIGURED_CHECK)
    {
        ConfigDefaults();
    }
    else
    {    
        MakeRandomSerialNumber(false);
    }

    GetConfiguredValues();

    LocoNet.init(LOCONET_TX_PIN);
    sv.init(VAL_LOCONET_MANUFACTURER, VAL_LOCONET_PRODUCTID, VAL_LOCONET_ARTNR, SOFTWARE_VERSION);
   
    #ifdef NATIVE_S88_ENABLED
        S88_Native.SetOnS88ReportEvent(&OnReportS88);
    #endif
    #ifdef  LOCAL_S88_ENABLED
        S88_Local.SetOnS88ReportEvent(&OnReportLocalS88);
    #endif

    #if defined(REQUIRES_I2C)
        Wire.begin();
        Wire.setWireTimeout(5000, true);
    #endif

    #ifdef AT24CXX_I2C_ADDR
        if (NVRAM::InitExternalEEPROM(AT24CXX_I2C_ADDR))
        {
            //LR_SPRNL(F("AR24C found"));
            bBools.bAT24CXXisPresent = 1;
        }
        else
        {
            //LR_SPRNL(F("notfound"));
            bBools.bAT24CXXisPresent = 0;
        }
    #endif

    #ifdef PCA9685_I2C_ADDR_START
        //Init 
        pca9685I2C.init(PCA9685_I2C_ADDR_START);
    #endif

    #ifdef LOCAL_I2C_S88_ENABLED
        S88_I2C.SetOnS88ReportEvent(&OnReportI2CS88);
    #endif    

    #ifdef VL53L0XVersion
        vlx.init();
        vlx.SetOnS88ReportEvent(&OnReportVL53LS88);
    #endif

    //Attach button press event to handler
    ButtonStopGo.OnButtonPressEvent(&OnButtonGoStopPress);
    #ifdef PROGRAM_BUTTON_ENABLED
        ButtonProgram.OnButtonPressEvent(&OnButtonProgramPress);
    #endif
       
    BoardLeds::Init();
    BoardLeds::SetSysLedON(25);

    /*
    if (!checkVersion())
    {        
        while (true)
        {         
            wdt_disable();
            BoardLeds::SetLedGo();
            delay(250);
            BoardLeds::SetLedStop();
            delay(250);
        }
    }*/

    BoardLeds::SetLedStopGo();
    delay(1000);
    BoardLeds::SetLedStopGoOff();
    delay(1000);
    NVRAM::SetOnSVChanged(notifySVChange);

    wdt_reset();
    wdt_enable(WDTO_2S);
}

void SetRunningStatus(bool status)
{
    if (status)
    {
        if (bBools.isJustBooted)
        {
            bBools.isJustBooted = false;
        }
        else
        {
            if (LoconetRunStatus < LocoRunStatus::LoconetProg_StartAddressState)
                BoardLeds::SetLedGo();

            #ifdef NATIVE_S88_ENABLED
                S88_Native.LoconetOnGo();
            #endif

            #ifdef LOCAL_S88_ENABLED
                S88_Local.LoconetOnGo();
            #endif

            #ifdef LOCAL_I2C_S88_ENABLED
                S88_I2C.LoconetOnGo();
            #endif      

            #ifdef  VL53L0XVersion
                vlx.LoconetOnGo();
            #endif

            //#ifdef PCA9685_I2C_ADDR_START
            //    ServoMgrI2C.ResetServoPinPositionRetrieved();
            //#endif

        }
    }
    else
    {
        if(LoconetRunStatus < LocoRunStatus::LoconetProg_StartAddressState)
            BoardLeds::SetLedStop();

        if (LoconetRunStatus != LocoRunStatus::LoconetStoppedLocalLedOnState && LoconetRunStatus != LocoRunStatus::LoconetStoppedLocalLedOffState)           
            LoconetRunStatus = LocoRunStatus::LoconetStopped;        

        #ifdef NATIVE_S88_ENABLED
            S88_Native.LoconetOnStop();
        #endif
        #ifdef LOCAL_S88_ENABLED
            S88_Local.LoconetOnStop();
        #endif

        #ifdef LOCAL_I2C_S88_ENABLED
            S88_I2C.LoconetOnStop();
        #endif

        #ifdef  VL53L0XVersion
            vlx.LoconetOnStop();
        #endif
    }
}

void SetLoconetBusyWait()
{
    LoconetBusyLastMillis = millis();
    LoconetBusyWaitTime = random(LOCONET_BUSY_WAIT_MS_MIN, LOCONET_BUSY_WAIT_MS_MAX);
}

void loop()
{
    wdt_reset();
    ProcessLoconetPackets();
    if (LoconetBusyLastMillis != 0 && (millis() - LoconetBusyLastMillis) > LoconetBusyWaitTime)
    {
        //Ensure to backoff until timer has expired. Clear LoconetBusyLastMillis once expired
        LoconetBusyLastMillis = 0;
    }

    if (bBools.bResend_Last_Loconet_Packet && LastReportedSensorAddress != 0)
    {
        LR_SPRNL('!');
        bBools.bResend_Last_Loconet_Packet = false;
        reportSensorState(LastReportedSensorAddress & 0x7FFF, LastReportedSensorAddress >> 15);
    }

    if (iLoopWaitCounter % 20 == 0)
    {
        #ifdef NATIVE_S88_ENABLED
            //Wait sending when just booted
            if (!bBools.isJustBooted)
            {
                if (S88_Native_StartIndex_Chain1 != 0)
                {             
                    S88_Native.Process();                
                }
            }
        #endif
    }

    #ifdef LOCAL_S88_ENABLED
        else if (iLoopWaitCounter % 20 == 1)
        {
            //Wait sending when just booted
            if (!bBools.isJustBooted)
            {
                if (S88_Local_StartIndex != 0)
                {
                    S88_Local.Process();
                }

            }
        }
    #endif

    #ifdef VL53L0XVersion
        else if (iLoopWaitCounter % 15 == 1)
        {
            //Wait sending when just booted
            if (!bBools.isJustBooted)
            {
                if (S88_VL53L0X_StartIndex != 0)
                {
                    vlx.Process();
                }
            }
        }
    #endif

    #ifdef LOCAL_I2C_S88_ENABLED
        else if (iLoopWaitCounter % 20 == 2)
        {
            if (!bBools.isJustBooted)
            {
                if (S88_I2C_StartIndex != 0)
                {
                    S88_I2C.Process();
                }
            }    
        }
    #endif
    else if (iLoopWaitCounter == LoopCounterWaiter::processLedAndProgramming)
    {
        BoardLeds::Process(LoconetRunStatus);
    }
    else if (iLoopWaitCounter == LoopCounterWaiter::processButtons)
    {
        ButtonStopGo.process();
        #ifdef PROGRAM_BUTTON_ENABLED
            ButtonProgram.process();    
        #endif
    }

    #ifdef PCA9685_I2C_ADDR_START
        pca9685I2C.process();
    #endif
    iLoopWaitCounter++;
}

bool notifySVWrite(uint16_t Offset, uint8_t& Value)
{
    if (Offset < SV_ADDR_SERIAL_NUMBER_H)
    {
        return false;
    }
    else if (Offset > SV_ADDR_SERIAL_NUMBER_H && Offset < SV_ADDR_SW_BUILDVER)
    {
        NVRAM::setSV(Offset, Value);
        /*#ifdef WITH_SERIAL  
            LR_SPRN(F("W-CV 0x")); LR_SPRNH(Offset); LR_SPRN(' '); LR_SPRNL(Value);
        #endif*/
    }
    #ifdef PCA9685_I2C_ADDR_START
        else if (Offset == SV_SERVO_FASTMODE)
        {
            ServoPositioningSV.MoveFast = Value;
        }
        else if (Offset == SV_SERVO_ID)
        {
            ServoPositioningSV.Index = Value;
        }
        else if (Offset == SV_SERVO_POS)
        {
            ServoPositioningSV.Position = Value;
            #ifdef  PCA9685_I2C_ADDR_START
                pca9685I2C.ChangeServoPosition(ServoPositioningSV);
            #endif
        }
    #endif //  #ifdef PCA9685_I2C_ADDR_START
    else
    {
        /*#ifdef WITH_SERIAL  
            LR_SPRN(F("WCV 0x")); LR_SPRNH(Offset);  LR_SPRN(' ');  LR_SPRNL(Value);
        #endif // WITH_SERIAL*/
    }
    return true;
}

void notifySVReconfigure()
{
    //LR_SPRNL(F("Recnf"));
    NVRAM::writeLNCV(CV_ACCESSORY_DECODER_CONFIGURED, VAL_ACCESSORY_DECODER_CONFIGURED_CHECK - 1);
    //After notifySVReconfigure, the device will be restarted, during restart the config is erased / reset
    delay(1000);
}

bool notifySVRead(uint16_t Offset, uint8_t& Value)
{   
    if (Offset <= SV_ADDR_SERIAL_NUMBER_H)
        return false;

    if (Offset == SV_MEMFREE_L)
    {    
        Value = NVRAM::freeRamMemory() & 0xFF;
    }
    else if (Offset == SV_MEMFREE_H)
    {
        Value = NVRAM::freeRamMemory() >> 8;
    }
    else if (Offset == SV_ADDR_SW_BUILDVER)
    {
        Value = VERSION_BUILD;
    }
    #ifdef PCA9685_I2C_ADDR_START
        else if (Offset == SV_EXTPWM_I2C)
        {
            Value = pca9685I2C.GetOnlineBoardsBitmaskValue();
        }
    #endif
    else if (Offset == SV_NVRAM_I2C)
    {
        Value = NVRAM::GetExternalMemSize();
    }
    else if (Offset == SV_ACCESSORY_BUILDFLAGS)
    {
        Value = HWCFG_BITMASK;
    }
    else if (Offset == SV_ADDR_HW_VER_QUERY)
    {
        Value = ACTIVE_HARDWARE_VERSION;
    }
    #ifdef REQUIRES_I2C
        else if(Offset >= SV_I2C_SCAN_START && Offset < SV_I2C_SCAN_END)
        {
            uint8_t startAddr = Offset - SV_I2C_SCAN_START;
            Value = I2CScan(startAddr * 8);
        }
    #endif
    #ifdef VL53L0XVersion
        else if (Offset >= SV_VL53L0X_SENSORREADING_START && Offset <= SV_VL53L0X_SENSORREADING_END)
        {
            uint8_t startAddr = Offset - SV_VL53L0X_SENSORREADING_START;
            Value = vlx.GetSensorCurrentValue(startAddr);
        }
    #endif    
    else if(Offset == SV_REPORTALL)
    {
        Value = 0x3F;
        #ifdef NATIVE_S88_ENABLED
            S88_Native.ReportAll();
        #endif

        #ifdef LOCAL_S88_ENABLED
            S88_Local.ReportAll();
        #endif

        #ifdef LOCAL_I2C_S88_ENABLED
            S88_I2C.ReportAll();
        #endif

        #ifdef VL53L0XVersion
            vlx.ReportAll();
        #endif
    }
    else if (Offset == SV_REBOOT_DEVICE)
    {
        while (1) {}            // stop and wait for watchdog to knock us out
    }
    else
    {
        Value = NVRAM::getSV(Offset);
    }
    //LR_SPRN(F("RCV 0x")); LR_SPRN(Offset, HEX); LR_SPRN(' '); LR_SPRNL(Value);   
    
    return true;
}

bool notifyCheckAddressRange(uint16_t startAddress, bool& AddressAccepted)
{
    AddressAccepted = true;
    return true;
}

#ifdef REQUIRES_I2C
    uint8_t I2CScan(const uint8_t &StartAddress)
    {
        uint8_t uiReturnValue;
        uint8_t Discovered_I2C_DeviceMask = 0;    
        //LR_SPRN(F("I2Cscan from: ")); LR_SPRNL(StartAddress, DEC);    
    
        if (StartAddress > 127)
        {
            //LR_SPRN(F("RanE"));        
            return -1;
        }

        for (uint8_t uiDeviceAddress = 0; uiDeviceAddress < 8; uiDeviceAddress++)
        {
            // The i2c_scanner uses the return value of
            // the Write.endTransmisstion to see if
            // a device did acknowledge to the address.

            if(I2CBase::I2C_CheckPWMDeviceOnAddress(uiDeviceAddress + StartAddress))
            {
                //LR_SPRN(F("I2C 0x")); LR_SPRNL(uiDeviceAddress + StartAddress, HEX);
                bitSet(Discovered_I2C_DeviceMask, 7 - uiDeviceAddress);
            }
            /*else if (uiReturnValue == 4)
            {
                LR_SPRN(F("Err 0x")); LR_SPRNL(uiDeviceAddress + StartAddress, HEX);
            }
            else
            {
                LR_SPRN(F("No 0x")); LR_SPRNL(uiDeviceAddress + StartAddress, HEX);
            }*/
        }
        //LR_SPRN("Mask:"); LR_SPRNL(founddevices);
        return Discovered_I2C_DeviceMask;
    }
#endif