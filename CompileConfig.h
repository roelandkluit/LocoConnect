#pragma once

//Hardware versions 1.0 and 1.2 are not publicly released. However, they are used on some private layouts.
//For compatibility reasons the pinout configs and compile options for this PCB versions are maintained
#define VERSION_1_0_PRINT_LAYOUT 1
#define VERSION_1_2_PRINT_LAYOUT 2
#define VERSION_1_3_PRINT_LAYOUT 3
#define VERSION_LCS1_PRINT_LAYOUT 3

//Start of Configuration section
//Due to max flash size, not all features can be enabled at the same time

//PCB version used. Normaly should be LCS1 / 1.3 -> HW version 3
#define ACTIVE_HARDWARE_VERSION VERSION_1_3_PRINT_LAYOUT

//Enables serial debugging, disabled by default to reduce RAM and Flash usage
#define WITH_SERIAL

//Enables the Native S88 Bus incl. RJ45 connector
#define NATIVE_S88_ENABLED

//Enables I2C additional NVRAM
#define NVRAM_I2C_ENABLED

//Enables the 4 local AUX pins for Feedback
#define LOCAL_S88_ENABLED

//Enables I2C occupation sensors
#define LOCAL_I2C_S88_ENABLED

//#Enabled Program Button
#define PROGRAM_BUTTON_ENABLED

//Enables VL53L0X Sensors for Feedback. Occupation Readings using a maximum of 8 I2C connected VL53L0X Sensors to PCA9548A board
// When the VL53L0X option is enabled
//  - The maximum of connected accessories will be limited to 32, due to the fact the NVRAM I2C module is disabled
//  - The local S88 connections are disabled
// This is because there is not enough flash to store all this code
//#define VL53L0XVersion

//End of Configuration section

#ifdef WITH_SERIAL
    #define LR_SPRN Serial.print
    #define LR_SPRNL Serial.println
    #define LR_SPRNH Serial.print
#else
    #define LR_SPRN(MSG)
    #define LR_SPRNL(MSG)
    #define LR_SPRNH(MSG)
#endif // WITH_SERIAL

#if ACTIVE_HARDWARE_VERSION == VERSION_1_0_PRINT_LAYOUT
    /*
        PinUsage:
        0   Serial
        1   Serial
        2   AUX
        3   S88-Clock
        4   S88-Load
        5   S88-Reset
        6   S88-Data
        7   LoconetTX
        8   LoconetRX
        9   N.C.
        10  N.C.
        11  AUX
        12  AUX
        13  LED SYS
        A0  AUX
        A1  AUX
        A2  AUX
        A3  Button Go\Stop
        A4  ProgramButton
        A5  LED Go\Stop
    */
#define S88_CLOCK 3
#define S88_LOAD 4
#define S88_RESET 5
#define S88_DATA1 6
#define S88_DATA2 0
#define AUXPIN1 11
#define AUXPIN2 12
#define AUXPIN3 2
#define AUXPIN4 A0
#define AUXPIN5 A1
#define AUXPIN6 A2
#define LOCONET_TX_PIN 7
#define LED_STOP_GO A5
#define BUTTON_PROGRAM A4
#define BUTTON_STOPGO A3
#define LED_SYS LED_BUILTIN
#undef NVRAM_I2C_ENABLED

#elif ACTIVE_HARDWARE_VERSION == VERSION_1_2_PRINT_LAYOUT
    /*
    PinUsage:
    0   Serial
    1   Serial
    2   AUX
    3   S88-Clock
    4   S88-Load
    5   S88-Reset
    6   S88-Data
    7   LoconetTX
    8   LoconetRX
    9   AUX
    10  AUX
    11  AUX
    12  AUX
    13  LED SYS
    A0  IR DETECT 1 //No longer used
    A1  IR DETECT 2 //No longer used
    A2  IR DETECT 3 //No longer used
    A3  Button Go\Stop
    A4  ProgramButton
    A5  LED Go\Stop
*/
#define S88_CLOCK 3
#define S88_LOAD 4
#define S88_RESET 5
#define S88_DATA1 6
#define S88_DATA2 0

#define AUXPIN1 2
#define AUXPIN2 9
#define AUXPIN3 10
#define AUXPIN4 11
#define AUXPIN5 12
#define LOCONET_TX_PIN 7
#define LED_STOP_GO A5
#define BUTTON_PROGRAM A4
#define BUTTON_STOPGO A3
#define LED_SYS LED_BUILTIN
#undef NVRAM_I2C_ENABLED

#elif ACTIVE_HARDWARE_VERSION == VERSION_LCS1_PRINT_LAYOUT
/*
PinUsage:
0   Serial
1   Serial
2   ProgramButton
3   S88-Clock
4   S88-Load
5   S88-Reset
6   S88-Data Chain 1
7   LoconetTX
8   LoconetRX
9   LED Go
10  LED Stop
11  AUX2 / MOSI
12  AUX1 / MISO
13  AUX3 / SCK
A0  S88-Data Chain 2
A1  SYSLED
A2  AUX4 / CS
A3  Button Go\Stop
A4  I2C SDA
A5  I2C SCL
*/
#define S88_CLOCK 3
#define S88_LOAD 4
#define S88_RESET 5
#define S88_DATA1 6
#define S88_DATA2 A0
#define AUXPIN1 12
#define AUXPIN2 11
#define AUXPIN3 13
#define AUXPIN4 A2
#define LOCONET_TX_PIN 7
#define LOCONET_RX_PIN 8
#define LED_STOP 10
#define LED_GO 9
#define BUTTON_PROGRAM 2
#define BUTTON_STOPGO A3
#define LED_SYS A1
//#define MAX_IR_REGISTERS 0
#define PCA9685_I2C_ADDR_START 0x40 + 0x20
#define PCF8574AT_I2C_ADDR 0x38
#define PCF8574T_I2C_ADDR 0x20
#define AT24CXX_I2C_ADDR 0x50
#endif

#define CV_ACCESSORY_DECODER_CONFIGURED 0x000E

#define SV_ADDR_SW_BUILDVER 0x8000
#define SV_ADDR_HW_VER_MEM 0x0007
#define SV_ADDR_HW_VER_QUERY 0x8001
#define SV_MEMFREE_L 0x8002
#define SV_MEMFREE_H 0x8003
#define SV_EXTPWM_I2C 0x8004
#define SV_NVRAM_I2C 0x8005

#define SV_REPORTALL 0x8030
#define SV_REBOOT_DEVICE 0x8034

#define CV_I2C_SCAN_START 0x4020 //CV --> 4020, SV = 0x803F,
#define CV_I2C_SCAN_END (CV_I2C_SCAN_START + 0x08) //8 --> CV = 0x4026, SV --> 0x804F

#define SV_I2C_SCAN_START CV_I2C_SCAN_START * 2
#define SV_I2C_SCAN_END CV_I2C_SCAN_END * 2

#define SV_VL53L0X_SENSORREADING_START 0x8050
#define SV_VL53L0X_SENSORREADING_END 0x8057

#define SV_SERVO_FASTMODE 0x8020
#define SV_SERVO_ID 0x8021
#define SV_SERVO_POS 0x8022

#define CV_ACCESSORY_DECODER_S88_START_ADDRES 0x0004
#define SV_ACCESSORY_DECODER_S88_START_ADDRES 0x0008

#define CV_ACCESSORY_DECODER_S88_ADDRES_COUNT 0x0005
#define SV_ACCESSORY_DECODER_S88_ADDRES_COUNT 0x000A

#define CV_ACCESSORY_DECODER_S88_CH2_START_ADDRES 0x0006
#define SV_ACCESSORY_DECODER_S88_CH2_START_ADDRES 0x000C

#define CV_ACCESSORY_DECODER_S88_CH2_ADDRES_COUNT 0x0007
#define SV_ACCESSORY_DECODER_S88_CH2_ADDRES_COUNT 0x000E

#define CV_ACCESSORY_DECODER_S88_I2C_START_ADDRES 0x0008
#define SV_ACCESSORY_DECODER_S88_I2C_START_ADDRES 0x0010

#define CV_ACCESSORY_DECODER_S88_I2C_ADDRES_COUNT 0x0009
#define SV_ACCESSORY_DECODER_S88_I2C_ADDRES_COUNT 0x0012

#define CV_ACCESSORY_DECODER_LOCAL_START_ADDRES 0x000A
#define SV_ACCESSORY_DECODER_LOCAL_START_ADDRES 0x0014

#define CV_ACCESSORY_DECODER_LOCAL_READ_COUNT 0x000B
#define SV_ACCESSORY_DECODER_LOCAL_READ_COUNT 0x0016

#define CV_ACCESSORY_DECODER_STOPGO_DEFAULTNC 0x000C
#define SV_ACCESSORY_DECODER_STOPGO_DEFAULTNC 0x0018

#define CV_ACCESSORY_DECODER_REPORT_BEHAVIOUR 0x000D
#define SV_ACCESSORY_DECODER_REPORT_BEHAVIOUR 0x001A

#define CV_ACCESSORY_VL53L0X_START_ADDRES 0x000F
#define SV_ACCESSORY_VL53L0X_START_ADDRES 0x001E

#define SV_ACCESSORY_VL53L0X_SENSORVAL_START 0x0020
#define SV_ACCESSORY_VL53L0X_SENSORVAL_END 0x0027

#define SV_ACCESSORY_BUILDFLAGS 0x8006

#define SV_MEM_STRING_NAME 0x0030
#define SV_MEM_STRING_END 0x0047 //Including

#define LOCONET_BUSY_WAIT_MS_MIN 4
#define LOCONET_BUSY_WAIT_MS_MAX 50
#define VAL_ACCESSORY_DECODER_CONFIGURED_CHECK 0x02

#define VAL_LOCONET_MANUFACTURER 0x0D //13
#define VAL_LOCONET_PRODUCTID 0xF0  //Makes F00D
#define VAL_LOCONET_ARTNR 2051

#ifdef VL53L0XVersion
    //See comment above
    #undef AT24CXX_I2C_ADDR
    #undef NVRAM_I2C_ENABLED
    #undef LOCAL_S88_ENABLED
#endif

#ifdef AT24CXX_I2C_ADDR
    #define MAX_NUMBER_OF_PWM_OUTPUT_PINS 64
#else
    #define MAX_NUMBER_OF_PWM_OUTPUT_PINS 32
#endif

#ifndef NVRAM_I2C_ENABLED
    #undef AT24CXX_I2C_ADDR    
#endif

#define PWM_PINS_PER_BOARD 16
#define MAX_NUMBER_OF_I2C_PWM_BOARDS (MAX_NUMBER_OF_PWM_OUTPUT_PINS / PWM_PINS_PER_BOARD)
#define SIZE_ACCESSORY_DECODER_I2C_BYTES_CONFIG 28
//OLD #define CV_ACCESSORY_DECODER_ASPECTS_DATA_START 0x0028
//OLD #define SV_ACCESSORY_DECODER_ASPECTS_DATA_START 0x0050 //SV 0x00050 --> CV 0x0028
#define CV_ACCESSORY_DECODER_ASPECTS_DATA_START 0x0040
#define SV_ACCESSORY_DECODER_ASPECTS_DATA_START 0x0080 //SV 0x00080 --> CV 0x0048
#define SV_ACCESSORY_DECODER_ASPECTS_DATA_END SV_ACCESSORY_DECODER_ASPECTS_DATA_START + (MAX_NUMBER_OF_PWM_OUTPUT_PINS * SIZE_ACCESSORY_DECODER_I2C_BYTES_CONFIG)

#if defined(PCA9685_I2C_ADDR_START) || defined(LOCAL_I2C_S88_ENABLED) || defined(VL53L0XVersion)
    #define REQUIRES_I2C
#endif

#ifdef NATIVE_S88_ENABLED
    #define HWCFG_BITMASK0 1
#else
    #define HWCFG_BITMASK0 0
#endif

#ifdef PCA9685_I2C_ADDR_START
    #define HWCFG_BITMASK1 2
#else
    #define HWCFG_BITMASK1 0
#endif

#ifdef PCF8574AT_I2C_ADDR
    #define HWCFG_BITMASK2 4
#else
    #define HWCFG_BITMASK2 0
#endif

#ifdef AT24CXX_I2C_ADDR
    #define HWCFG_BITMASK3 8
#else
    #define HWCFG_BITMASK3 0
#endif

#ifdef VL53L0XVersion
    #define HWCFG_BITMASK4 16
#else
    #define HWCFG_BITMASK4 0
#endif

#ifdef LOCAL_S88_ENABLED
    #define HWCFG_BITMASK5 32
#else
    #define HWCFG_BITMASK5 0
#endif

#define HWCFG_BITMASK6 0
//#define HWCFG_BITMASK6 64
#define HWCFG_BITMASK7 0
//#define HWCFG_BITMASK7 128

#define HWCFG_BITMASK HWCFG_BITMASK0 + HWCFG_BITMASK1 + HWCFG_BITMASK2 + HWCFG_BITMASK3 + HWCFG_BITMASK4 + HWCFG_BITMASK5 + HWCFG_BITMASK6 + HWCFG_BITMASK7
