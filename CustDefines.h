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

const char DEFAULT_DEVICE_NAME_STRING[] PROGMEM = { "Epty-\x0A9Rkl" };

enum LoopCounterWaiter : uint8_t
{
    processLedAndProgramming = 110,
    processButtons = 128,
};

enum S88ReportingOptions : uint8_t
{
    S88ReportOnlyWhenOnGo = 0,
    S88ReportAlwaysReport = 1,
    S88ReportAlwaysReportAndRefreshOnGo = 2,
};

enum LocoRunStatus : uint8_t
{
    LoconetInitialLedOffState = 0, //Board is booted but loconet status is unkown
    LoconetInitialLedOnState = 1,
    LoconetStopped = 2, //Loconet is stopped
    LoconetStoppedLocalLedOffState = 3, //Loconet is stopped using button on PCB
    LoconetStoppedLocalLedOnState = 4,
    LoconetGo = 8, //Loconet is in running mode
    LoconetProg_StartAddressState = 0x80,
    LoconetProg_StartModCountState = 0x82,
};