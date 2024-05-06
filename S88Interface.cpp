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
#include "S88Interface.h"
#ifdef REQUIRES_I2C
    #include <Wire.h>
#endif // REQUIRES_I2C

//Reference for ISR callback
S88Interface* _ptrISR_Callback_NativeS88_interface = NULL;

//I2C Interface
S88Interface::S88Interface(const uint8_t &I2CAddress)
{
    PIN_DATA = I2CAddress;
    //PIN_DATA_2 = 0;
    PIN_CLOCK = 0;
    PIN_LOAD = 0;
    PIN_RESET = 0;

    initClass();
}

//S88 Native
S88Interface::S88Interface(const uint8_t &pinData, const uint8_t &pinClock, const uint8_t &pinLoad, const uint8_t &pinReset)// , uint8_t pinData2)
{
    if (_ptrISR_Callback_NativeS88_interface != NULL)
    {
        //Cannot have multiple Timer2 instances, therefor only one native S88 class!
        //Todo: Implement pinData2 if multiple are needed
        return;
    }

    PIN_DATA = pinData;
    PIN_CLOCK = pinClock;
    PIN_LOAD = pinLoad;
    PIN_RESET = pinReset;
    //PIN_DATA_2 = pinData2;

    pinMode(PIN_DATA, INPUT);
    /*if (PIN_DATA_2 != 0)
    {
        pinMode(PIN_DATA_2, INPUT);
        //digitalWrite(PIN_DATA_2, LOW);// Pullup disabled
    }*/

    pinMode(PIN_CLOCK, OUTPUT);
    pinMode(PIN_LOAD, OUTPUT);
    pinMode(PIN_RESET, OUTPUT);

    initClass();
    _ptrISR_Callback_NativeS88_interface = this;

    //Set Timer2 for S88 Native
    cli();
    TCCR2A = 1 << WGM21;
    TCCR2B = (1 << CS22);// | (0 << CS21) | (0 << CS20);
    TIMSK2 = (1 << OCIE2A);
    OCR2A = 0x10;
    sei();
}

void S88Interface::initClass()
{
    timing = S88TimingEnum::S88_PHASE_IDLE;    

    s88DataValues = new byte[MAX_S88_REGISTERS];
    s88PendingReport = new byte[MAX_S88_REGISTERS];

    for (uint8_t i = 0; i < MAX_S88_REGISTERS; i++)
    {
        s88DataValues[i] = 0;
        s88PendingReport[i] = 0;
    }
}

uint8_t S88Interface::GetMaxModuleCount()
{
    return MAX_S88_REGISTERS;
}

void S88Interface::SetS88moduleCount(uint8_t &moduleCount)
{
    if (moduleCount > MAX_S88_REGISTERS)
    {
        moduleCount = MAX_S88_REGISTERS;
    }

    S88moduleCount = moduleCount;
}

uint8_t S88Interface::GetS88moduleCount()
{
    return S88moduleCount;
}

void S88Interface::ReportAll()
{
    ReportAll(0);
}

void S88Interface::ReportAll(const uint8_t startindex = 0)
{
    uint8_t modulesToReport = S88moduleCount;
    if (reportOnlyActiveModules)
        modulesToReport = S88ActiveModules;

    for (uint8_t i = startindex; i <= modulesToReport; i++)
    {
        s88PendingReport[i] = 0xFF;
    }
}

void S88Interface::UpdateBit(const uint8_t& pos, const uint8_t& readBitValue)
{
    uint8_t index = pos & 0x07;
    uint8_t arrayindex = pos >> 3;

    uint8_t isPendingReport = bitRead(s88PendingReport[arrayindex], index);
    uint8_t currentValue = bitRead(s88DataValues[arrayindex], index);

    if (currentValue == 1 && isPendingReport == 1)
    {
        //No update, report of high value is pending
    }
    else if (readBitValue != currentValue)
    {        
        if (S88ActiveModules < arrayindex)
        {
            uint8_t curVal = S88ActiveModules;
            S88ActiveModules = arrayindex;
            ReportAll(curVal);
        }

        if (readBitValue == 0)
        {
            bitClear(s88DataValues[arrayindex], index);
        }
        else
        {
            bitSet(s88DataValues[arrayindex], index);
        }
        bitSet(s88PendingReport[arrayindex], index);
    }
}

void S88Interface::__process_ISR__Tick()
{
    if (timing == S88TimingEnum::S88_PHASE_ISR_BUSY)
        return;

    S88TimingEnum tmp = timing;

    switch (timing)
    {
    case S88TimingEnum::S88_PHASE_DONE:
    case S88TimingEnum::S88_PHASE_IDLE:
        timing = tmp;
        break;

    //S88 restart reading
    case S88TimingEnum::S88_PHASE_A1_HLOAD:
    {
        S88ChainReadPosition = 0;
        digitalWrite(PIN_LOAD, HIGH);
        timing = S88TimingEnum::S88_PHASE_A2_HCLOCK;
        break;
    }

    case S88TimingEnum::S88_PHASE_A2_HCLOCK:
    {
        digitalWrite(PIN_CLOCK, HIGH);
        timing = S88TimingEnum::S88_PHASE_A3_LCLOCK;
        break;
    }

    case S88TimingEnum::S88_PHASE_A3_LCLOCK:
    {
        digitalWrite(PIN_CLOCK, LOW);
        timing = S88TimingEnum::S88_PHASE_A4_LLOAD;
        break;
    }

    case S88TimingEnum::S88_PHASE_A4_LLOAD:
    {
        digitalWrite(PIN_LOAD, LOW);
        timing = S88TimingEnum::S88_PHASE_B1_READ;
        break;
    }

    //S88 read bit
    case S88TimingEnum::S88_PHASE_B1_READ:
    {
        uint8_t currentBitValue1 = digitalRead(PIN_DATA);

        UpdateBit(S88ChainReadPosition, currentBitValue1);
        S88ChainReadPosition++;

        if (S88ChainReadPosition >= (S88moduleCount * 8))
        {
            //Read of all units completed, reset chain
            timing = S88TimingEnum::S88_PHASE_R1_HLOAD;
        }
        else
        {
            timing = S88TimingEnum::S88_PHASE_B2_HCLOCK;
        }
        break;
    }

    //S88 next bit
    case S88TimingEnum::S88_PHASE_B2_HCLOCK:
    {
        digitalWrite(PIN_CLOCK, HIGH);
        timing = S88TimingEnum::S88_PHASE_B3_LCLOCK;
        break;
    }

    case S88TimingEnum::S88_PHASE_B3_LCLOCK:
    {
        digitalWrite(PIN_CLOCK, LOW); 
        timing = S88TimingEnum::S88_PHASE_B1_READ;
        break;
    }

    //S88 values Reset
    case S88TimingEnum::S88_PHASE_R1_HLOAD:
    {
        digitalWrite(PIN_LOAD, HIGH);
        timing = S88TimingEnum::S88_PHASE_R2_HRESET;
        break;
    }

    case S88TimingEnum::S88_PHASE_R2_HRESET:
    {
        digitalWrite(PIN_RESET, HIGH);
        timing = S88TimingEnum::S88_PHASE_R3_LRESET;
        break;
    }

    case S88TimingEnum::S88_PHASE_R3_LRESET:
    {
        digitalWrite(PIN_RESET, LOW);
        timing = S88TimingEnum::S88_PHASE_R4_LLOAD;
        break;
    }

    case S88TimingEnum::S88_PHASE_R4_LLOAD:
    {
        digitalWrite(PIN_LOAD, LOW);
        timing = S88TimingEnum::S88_PHASE_DONE;
        break;
    }

    default:
        timing = tmp;
        break;
    }
}

void S88Interface::ChangeI2C_StartAddr(const uint8_t& I2CAddress)
{
    if (PIN_CLOCK == 0)
    {
        PIN_DATA = I2CAddress;
    }
}

void S88Interface::Process()
{
    if ((isLoconetGo == 1) || S88ReportingBehavour != S88ReportingOptions::S88ReportOnlyWhenOnGo)
    {
        if ((millis() - previousRefreshMillis) >= S88_REFRESH_INTERVAL)
        {
            if (PIN_CLOCK != 0)
            {
                if (timing == S88TimingEnum::S88_PHASE_IDLE)
                {
                    timing = S88TimingEnum::S88_PHASE_START;
                }
                else if (timing == S88TimingEnum::S88_PHASE_DONE)
                {
                    timing = S88TimingEnum::S88_PHASE_IDLE;
                    previousRefreshMillis = millis();
                }
            }
            else
            {
                #ifdef REQUIRES_I2C
                    if(RefreshS88DataI2C()) //returns true if all devices have been checked;
                                            //returns false if more I2C device readings are pending
                    {
                        previousRefreshMillis = millis();
                    }                    
                #endif
            }
        }
        if ((millis() - previousSendMillis) >= S88_UPDATE_INTERPACK_GAP)
        {
            previousSendMillis = millis();
            if (noReportYet > 0)
            {
                noReportYet--;
            }
            else
            {
                GetNextItemToReport();
            }

        }
    }
}

bool S88Interface::ProcessReportItems(const bool &ReportActiveAndInactivePins)
{
    for (uint8_t i = 0; i < S88moduleCount; i++)
    {
        //The pending report mask is 0 if there is no pin to report
        if (s88PendingReport[i] != 0)
        {
            //Ok, we have at least one pin to report, check the pins (8 bits)
            for (uint8_t bitindex = 0; bitindex < 8; bitindex++)
            {
                //Calculate the mask for the pin
                uint8_t bitmask = 1 << bitindex;
                //Check if report is pending for this pin
                if (s88PendingReport[i] & bitmask)
                {
                    //Report the active values that have changed if ReportActiveAndInactivePins = false
                    if (ReportActiveAndInactivePins || (s88DataValues[i] & bitmask) != 0)
                    {
                        bool succesResult = false;
                        //Found it, now report. Callback definition is checked at top of function
                        __SD_CALLBACK_S88REPORT(bitindex + (i * 8), (s88DataValues[i] & bitmask) != 0, succesResult);
                        if (succesResult)
                        {
                            //Remove bit from bits to report mask
                            s88PendingReport[i] &= ~bitmask;
                        }
                        else
                        {
                            //hold back a bit, before retrying
                            if(ReportActiveAndInactivePins)
                                noReportYet = random(1, 5); //When processing all pins, allow more time to allow active pin reports to prioritize
                            else                            
                                noReportYet = random(0, 3);
                        }
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

#ifdef REQUIRES_I2C
bool S88Interface::RefreshS88DataI2C()
{
    if (S88ChainReadPosition < S88moduleCount)
    {
        uint8_t DeviceAddr = PIN_DATA + S88ChainReadPosition;
        uint8_t newValues = 0x00;
        if (I2C_CheckPWMDeviceOnAddress(DeviceAddr))
        {
            if (Wire.requestFrom((int)(DeviceAddr), 1))
            {
                newValues = Wire.read();
            }
            //Enable internall pull-up
            Wire.beginTransmission(DeviceAddr);
            Wire.write(0xFF);
            Wire.endTransmission(DeviceAddr);
        }
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            UpdateBit(((S88ChainReadPosition * 8) + bit), (bitRead(newValues, bit) ? 0 : 1));
        }
        S88ChainReadPosition++;
    }
    else
    {
        S88ChainReadPosition = 0;
        return true;
    }
    return false;
}
#endif

ISR(TIMER2_COMPA_vect)
{
    sei(); //Re-enabled interrupts (ISR Nesting)
          //This to allow Loconet Packets to succesfully arrive
          //This function shall never take longer than 1/2 * timer interval of ISR

    if (_ptrISR_Callback_NativeS88_interface != NULL)
    {
        _ptrISR_Callback_NativeS88_interface->__process_ISR__Tick();
    }
}