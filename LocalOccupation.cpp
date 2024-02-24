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

#include "LocalOccupation.h"
#include "CompileConfig.h"

LocalOccupation::LocalOccupation(byte pins[], byte pin_count)
{
	if (pin_count > 8)
		pin_count = 8;

	enabledPinCount = 0;
	capablePinCount = pin_count;
	pin_activeTime = new byte[capablePinCount];
	pin_status = 0;
	pin_ioport = pins;
	previousRefreshMillis = millis() + LOCAL_OCC_REFRESH_INTERVAL;
	previousSendMillis = millis() + LOCAL_OCC_UPDATE_INTERPACK_GAP;
}

void LocalOccupation::SetEnabledPins(byte count)
{
	if (count > capablePinCount)
	{
		count = capablePinCount;
	}
	enabledPinCount = count;

	for (int i = 0; i < enabledPinCount; i++)
	{
		pinMode(pin_ioport[i], INPUT_PULLUP);
		pin_activeTime[i] = ACTIVATION_DELAY_MULTIPLY;
	}
}

void LocalOccupation::Process()
{
	if ((isLoconetGo == 1) || S88ReportingBehavour != S88ReportingOptions::S88ReportOnlyWhenOnGo)
	{
		if ((millis() - previousRefreshMillis) >= LOCAL_OCC_REFRESH_INTERVAL)
		{
			previousRefreshMillis = millis();
			UpdatePinStatus();
		}
		if ((millis() - previousSendMillis) >= LOCAL_OCC_UPDATE_INTERPACK_GAP)
		{
			previousSendMillis = millis();
			GetNextItemToReport();
		}
	}
}

bool LocalOccupation::ProcessReportItems(const bool& ReportActiveAndInactivePins)
{
	//The pending report mask is 0 if there is no pin to report
	if (s88PendingReport != 0)
	{
		//Update bitmask to signal we have recieved data for this module, and will be included in the refresh
		//setReportingBit(i);

		uint8_t index = 8;
		//Ok, we have at least one pin to report, check the pins
		for (uint16_t mask = (1 << CHAR_BIT); mask >>= 1; )
		{
			if (index > enabledPinCount)
			{
				s88PendingReport &= ~mask;
				//skip
			}
			else if (s88PendingReport & mask)
			{
				//Report the active values that have changed if ReportActiveAndInactivePins = false
				if (ReportActiveAndInactivePins || (pin_status & mask) != 0)
				{
					bool succesResult = false;
					//Found it, now report. Callback definition is checked at top of function
					__SD_CALLBACK_S88REPORT(index - 1, (pin_status & mask) != 0, succesResult);
					if (succesResult)
					{
						//Remove bit from mask
						s88PendingReport &= ~mask;
					}
					return true;
				}
			}
			index--;
		}
	}
	return false;
}

void LocalOccupation::UpdatePinStatus()
{
	for (int i = 0; i < enabledPinCount; i++)
	{
		byte val = !digitalRead(pin_ioport[i]);

		if (val == HIGH)
		{
			if (bitRead(pin_status, i) != val)
			{
				//LR_SPRN("Occupied: ");
				//LR_SPRNL(i);
				bitSet(s88PendingReport, i);
				bitSet(pin_status, i);
			}
			pin_activeTime[i] = ACTIVATION_DELAY_MULTIPLY;
		}
		else
		{
			if (bitRead(pin_status, i) != val)
			{
				if (pin_activeTime[i] == 0)
				{
					//LR_SPRN("Freed: ");
					//LR_SPRNL(i);
					bitClear(pin_status, i);
					bitSet(s88PendingReport, i);
				}
				else
				{
					pin_activeTime[i]--;
				}
			}
		}
	}
}