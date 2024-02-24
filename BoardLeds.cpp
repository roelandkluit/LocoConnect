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

#include "BoardLeds.h"
#include "CompileConfig.h"

uint8_t BoardLeds::Programblink = 0;
uint8_t BoardLeds::RemainingLedOnTime = 0;

void BoardLeds::SetSysLedON(const uint8_t& timeon)
{
	RemainingLedOnTime = 25;
	digitalWrite(LED_SYS, HIGH);
}

void BoardLeds::SetLedStop()
{
#ifdef LED_STOP_GO
    digitalWrite(LED_STOP_GO, 0);
#else
    digitalWrite(LED_GO, LOW);
    digitalWrite(LED_STOP, HIGH);
#endif
}

void BoardLeds::SetLedGo()
{
#ifdef LED_STOP_GO
    digitalWrite(LED_STOP_GO, 255);
#else
    digitalWrite(LED_GO, HIGH);
    digitalWrite(LED_STOP, LOW);
#endif
}

void BoardLeds::Init()
{
	pinMode(LED_SYS, OUTPUT);
	#ifdef LED_STOP_GO
		pinMode(LED_STOP_GO, OUTPUT);
	#endif // LED_STOP_GO
	#ifdef  LED_GO
		pinMode(LED_GO, OUTPUT);
	#endif //  LED_GO
	#ifdef  LED_STOP
		pinMode(LED_STOP, OUTPUT);
	#endif //  LED_GO
}

void BoardLeds::Process(LocoRunStatus &LocoRunMode)
{
	if (RemainingLedOnTime == 0)
	{
	}
	else if (RemainingLedOnTime == 1)
	{
		RemainingLedOnTime = 0;
		digitalWrite(LED_SYS, LOW);
	}
	else
	{
		RemainingLedOnTime--;
	}

    if (LocoRunMode == LocoRunStatus::LoconetProg_StartAddressState)
    {
        if (Programblink == 0)
        {
            SetLedGo();
        }
        else if (Programblink == 100)
        {
            SetLedStop();
        }
        else if (Programblink >= 200)
        {
            Programblink = -1;
        }
        Programblink++;
    }
    else if (LocoRunMode == LocoRunStatus::LoconetProg_StartModCountState)
    {
        if (Programblink == 0)
        {
            SetLedGo();
        }
        else if (Programblink == 50)
        {
            SetLedStop();
        }
        else if (Programblink >= 100)
        {
            Programblink = -1;
        }
        Programblink++;
    }
    else
    {
        //Led blink actions
        #ifndef LED_STOP_GO
        if (Programblink == 0)
        {
            Programblink = 200;
            if (LocoRunMode == LocoRunStatus::LoconetInitialLedOnState)
            {
                digitalWrite(LED_GO, LOW);
                digitalWrite(LED_SYS, LOW);
                LocoRunMode = LocoRunStatus::LoconetInitialLedOffState;
            }
            else if (LocoRunMode == LocoRunStatus::LoconetInitialLedOffState)
            {
                digitalWrite(LED_GO, HIGH);
                digitalWrite(LED_STOP, LOW);
                digitalWrite(LED_SYS, LOW);
                LocoRunMode = LocoRunStatus::LoconetInitialLedOnState;
            }
            else if (LocoRunMode == LocoRunStatus::LoconetStoppedLocalLedOnState)
            {
                digitalWrite(LED_STOP, LOW);
                digitalWrite(LED_SYS, LOW);
                LocoRunMode = LocoRunStatus::LoconetStoppedLocalLedOffState;
            }
            else if (LocoRunMode == LocoRunStatus::LoconetStoppedLocalLedOffState)
            {
                digitalWrite(LED_GO, LOW);
                digitalWrite(LED_STOP, HIGH);
                digitalWrite(LED_SYS, LOW);
                LocoRunMode = LocoRunStatus::LoconetStoppedLocalLedOnState;
            }
        }
        else
        {
            Programblink--;
        }
    #endif
    }
}
