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
#ifndef _LOCALOCCUPATION_h
#define _LOCALOCCUPATION_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
#include "CustDefines.h"
#include "S88Base.h"

#define LOCAL_OCC_REFRESH_INTERVAL 10
#define LOCAL_OCC_UPDATE_INTERPACK_GAP 5
#define ACTIVATION_DELAY_MULTIPLY 50 // multiply --> LOCAL_OCC_REFRESH_INTERVAL * ACTIVATION_DELAY_MULTIPLY ==> milisconds to delay reporting contact release
#define CHAR_BIT 8

class LocalOccupation: public S88Base
{
private:
	byte* pin_ioport;
	unsigned enabledPinCount:4;
	unsigned capablePinCount:4;
	byte* pin_activeTime;
	byte pin_status;
	byte s88PendingReport = 0;
	void UpdatePinStatus();
	bool ProcessReportItems(const bool& ReportActiveAndInactivePins) override;
public:
	void ReportAll() override { s88PendingReport = 0xFF; };
	LocalOccupation(byte pins[], byte size);
	void SetEnabledPins(byte count);
	void Process();	
};
#endif