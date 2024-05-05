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

#ifndef _S88INTERFACE_h
#define _S88INTERFACE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
#include "CustDefines.h"
#include "I2CBase.h"
#include "S88Base.h"

#define MAX_S88_REGISTERS 16
#define S88_RESET_INTERVAL 5
#define S88_REFRESH_INTERVAL 50
#define S88_UPDATE_INTERPACK_GAP 5

class S88Interface : I2CBase, public S88Base
{
private:
	uint8_t PIN_DATA = 6;
	uint8_t PIN_CLOCK = 3;
	uint8_t PIN_LOAD = 4;
	uint8_t PIN_RESET = 5;
	//uint8_t PIN_DATA_2 = A1;
	/**
	 * The number of contacts available.
	 */
	volatile uint8_t S88moduleCount = 0;

	/**
	 * Setting to cap reporting of S88 modules to the maximum amount of modules set.
	*/
	uint8_t S88ActiveModules = 0;

	/**
	* If active, report S88 changes
	*/
	uint8_t noReportYet = 0;
	/**
	 * Contact values
	 */
	volatile byte* s88DataValues;
	/**
	 * Pending report values
	 */
	volatile byte *s88PendingReport;
	/**
	* Data pointers values for previous data values
	*/
	void UpdateBit(const uint8_t& pos, const uint8_t& readBitValue);
	bool RefreshS88DataI2C();
	bool ProcessReportItems(const bool &ReportActiveAndInactivePins) override;
	enum class S88TimingEnum : uint8_t
	{
		S88_PHASE_START = 0,
		S88_PHASE_A1_HLOAD = 0,
		S88_PHASE_A2_HCLOCK = 1,
		S88_PHASE_A3_LCLOCK = 2,
		S88_PHASE_A4_LLOAD = 3,
		S88_PHASE_B1_READ = 4,
		S88_PHASE_B2_HCLOCK = 5,
		S88_PHASE_B3_LCLOCK = 6,

		S88_PHASE_IDLE = 7,

		S88_PHASE_RESET = 8,
		S88_PHASE_R1_HLOAD = 8,
		S88_PHASE_R2_HRESET = 9,
		S88_PHASE_R3_LRESET = 10,
		S88_PHASE_R4_LLOAD = 11,
		
		S88_PHASE_DONE = 12,

		S88_PHASE_ISR_BUSY = 13,
	};

	S88TimingEnum timing : 4;	
	uint8_t S88ChainReadPosition = 0;
	void ReportAll(const uint8_t startindex);
public:	
	//bool HasPendingReportItems();
	bool reportOnlyActiveModules = true;
	/**
	 * Creates a new S88 Interface
	 */
	 /**
	 * I2C Interface version
	 */
	S88Interface(const uint8_t &I2CAddress);
	/**
	* regular S88 Interface version
	*/
	S88Interface(const uint8_t& pinData, const uint8_t& pinClock, const uint8_t& pinLoad, const uint8_t& pinReset);// , uint8_t pinData2);
	void initClass();
	uint8_t GetMaxModuleCount();
	/**
	* Set Module Count with the given number of modules
	* being attached. While this value can be safely set to the
	* maximum of 32, it makes sense to specify the actual number,
	* since this speeds up reporting. The method assumes 8 bit
	* modules.
	*/
	void SetS88moduleCount(uint8_t& moduleCount);
	uint8_t GetS88moduleCount();
	/**
	 * Reads the current state of all contacts into the TrackReporter
	 * and clears the flip-flops on all S88 boards. Call this method
	 * periodically to have up-to-date values.
	 */
	void ReportAll() override;	
	/**
	* Call often to ensure S88 status gets updated
	*/
	void Process();
	void __process_ISR__Tick();
	void ChangeI2C_StartAddr(const uint8_t& I2CAddress);
};

#endif