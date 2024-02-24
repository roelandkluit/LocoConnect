# LocoConnect
LocoConnect controller board Firmware

## Compilation

- Install Arduino IDE 2.0 or later
- install the required depandancies
- Download the source code
- Compile and upload

## Firmware upload using SD card

**Requires the LocoConnect bootloader to be installed ** https://github.com/LocoConnect/LCBootloader/releases<br />
Convert Hex output file to binary file:<br />
```shell
%localappdata%\arduino15\packages\arduino\tools\avr-gcc\7.3.0-atmel3.6.1-arduino7\avr\bin\objcopy.exe -I ihex -O binary LocoConnect.ino.hex LCONNECT.BIN
```
Copy the LCONNECT.BIN file to the SD Card<br />
Attach the SD Card to the SPI connector of the PCB

## Features

Todo

## LocoConnect PCB and components

Todo

## External hardware

Todo

## Configuration

The configuration of the controller is performed over LocoNet. Download and run the https://github.com/LocoConnect/LocoProgrammer.

## Dependancies
All Versions - 
name=LocoNet
version=1.1.13
author=Alex Shepherd, John Plocher, Damian Philipp, Tom Knox, Hans Tanner, Bj�rn Rennfanz
url=https://github.com/mrrwa/LocoNet

VL53 Version -
name=VL53L0X
version=1.3.1
author=Pololu
url=https://github.com/pololu/vl53l0x-arduino


## License
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
