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
#include "S88Base.h"

byte S88Base::S88ReportingBehavour = S88ReportingOptions::S88ReportAlwaysReport;

/*
void S88Base::ReportAll()
{
}*/

void S88Base::LoconetOnStop()
{
	if (isLoconetGo == 1)
	{
		//SPRNL("S88 Stopped");
		isLoconetGo = 0;
	}
}

void S88Base::LoconetOnGo()
{
	if (isLoconetGo == 0)
	{
		//SPRNL("S88 Resumed");
		if (S88ReportingBehavour != S88ReportingOptions::S88ReportAlwaysReport)
		{
			ReportAll();
			previousRefreshMillis = millis();
			previousSendMillis = millis();
		}
		isLoconetGo = 1;
	}
}

/*
bool S88Base::ProcessReportItems(const bool& ReportActiveAndInactivePins)
{
	return false;
}*/

void S88Base::GetNextItemToReport()
{
	if (__SD_CALLBACK_S88REPORT == NULL)
		return;

	//First process Active pins only
	if (ProcessReportItems(false))
		return;

	//Process All pins
	ProcessReportItems(true);
}
