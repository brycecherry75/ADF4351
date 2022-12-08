# ADF4351
Arduino Library for the ADF4351 Wideband Frequency Synthesizer chip

v1.0.0 First release

v1.0.1 Double buffering of RF frequency divider implemented by default

v1.1.0 Added current frequency read function

v1.1.1 Corrected issue with conversion in ReadCurrentFreq

v1.1.2 Add setPowerLevel function which can be used for frequency bursts

v1.1.3 Added direct entry of frequency parameters for precalculated frequencies of the highest possible precision

## Introduction

This library supports the ADF4351 from Analog Devices on Arduinos. The chip is a wideband (34.375 MHz to 4.4 GHz) Phase-Locked Loop (PLL) and Voltage Controlled Oscillator (VCO), covering a very wide frequency range under digital control. Just add an external PLL loop filter, Reference frequency source and a power supply for a very useful frequency generator for applications as a Local Oscillator or Sweep Generator.  

The chip generates the frequency using a programmable Fractional-N and Integer-N Phase-Locked Loop (PLL) and Voltage Controlled Oscillator (VCO) with an external loop filter and frequency reference. The chip is controlled by 
a SPI interface, which is controlled by a microcontroller such as the Arduino.

The library provides an SPI control interface for the ADF4351, and also provides functions to calculate and set the frequency, which greatly simplifies the integration of this chip into a design. The calculations are done using the excellent [Big Number Arduino Library](https://github.com/nickgammon/BigNumber) by Nick Gammon. The library also exposes all of the PLL variables, such as FRAC, Mod and INT, so they examined as needed.  

Requires the BitFieldManipulation library: http://github.com/brycecherry75/BitFieldManipulation
Requires the BeyondByte library: http://github.com/brycecherry75/BeyondByte

A low phase noise stable oscillator is required for this module. Typically, an Ovenized Crystal Oscillator (OCXO) in the 10 MHz to 100 MHz range is used.  

## Features

+ Frequency Range: 34.375 MHz to 4.4 GHz
+ Output Level: -4 dBm to 5 dBm (in 3 dB steps) 
+ In-Band Phase Noise: -100 dBc/Hz (3 kHz from 2.1 Ghz carrier)
+ PLL Modes: Fraction-N and Integer-N (set automatically)
+ Signal On/Off control
+ All ADF4351_R[] registers can be accessed and manipulated

## Library Use

An example program using the library is provided in the source directory [example4351.ino](src/example4351.ino).

init(SSpin, LockPinNumber, Lock_Pin_Used, CEpin, CE_Pin_Used): initialize the ADF4351 with SPI SS pin, lock pin and true/false for lock pin use and CE pin use - CE pin is typically LOW (disabled) on reset if used; depending on your board, this pin along with the RF Power Down pin may have a pullup or pulldown resistor fitted and certain boards have the RF Power Down pin (low active) on the header

SetStepFreq(frequency): sets the step frequency in Hz - default is 100 kHz - returns an error code

ReadR()/ReadInt()/ReadFraction()/ReadMod(): returns a uint16_t value for the currently programmed register

ReadOutDivider()/ReadOutDivider_PowerOf2()/ReadRDIV2()/ReadRefDoubler(): returns a uint8_t value for the currently programmed register - ReadOutDivider() is automatically converted from a binary exponent to an actual division ratio and 
ReadOutDivider_PowerOf2() is a binary exponent

ReadPFDfreq(): returns a double for the PFD value

setf(*frequency, PowerLevel, AuxPowerLevel, AuxFrequencyDivider, PrecisionFrequency, FrequencyTolerance, CalculationTimeout): set the frequency (in Hz with char string - decimal places will be ignored) power level/auxiliary power level (1-4 in 3dBm steps from -5dBm or 0 to disable), mode for auxiliary frequency output (ADF4351_AUX_(DIVIDED/FUNDAMENTAL)), true/false for precision frequency mode (step size is ignored if true), frequency tolerance (in Hz with uint32_t) under precision frequency mode (rounded to the nearest integer), calculation timeout (in mS with uint32_t - recommended value is 45000 in most cases, 0 to disable) under precision frequency mode - returns an error or warning code

setrf(frequency, R_divider, ReferenceDivisionType): set the reference frequency and reference divider R and reference frequency division type (ADF4351_REF_(UNDIVIDED/HALF/DOUBLE)) - default is 10 MHz/1/undivided - returns an error code

setfDirect(R_divider, INT_value, MOD_value, FRAC_value, RF_DIVIDER_value, FRACTIONAL_MODE): RF divider value is (1/2/4/8/16/32/64) and fractional mode is a true/false bool - these paramaters will not be checked for invalid values

setPowerLevel/setAuxPowerLevel(PowerLevel): set the power level (0 to disable or 1-4) and write to the ADF4355 in one operation - returns an error code

WriteSweepValues(*regs): high speed write for registers when used for frequency sweep (*regs is uint32_t and size is as per ADF4351_RegsToWrite)

ReadSweepValues(*regs): high speed read for registers when used for frequency sweep (*regs is uint32_t and size is as per ADF4351_RegsToWrite)

ReadCurrentFreq(*freq): calculation of currently programmed frequency (*freq is uint8_t and size is as per ADF4351_ReadCurrentFrequency_ArraySize)

A Python script (ADF4351pf.py) can be used for calculating the required values for setfDirect for speed.

Please note that you should install the provided BigNumber library in your Arduino library directory.

Under worst possible conditions (tested with 2.200006102 GHz RF/25 MHz PFD/0 Hz tolerance target error which will go through the entire permissible range of MOD values) on a 16 MHz AVR Arduino, precision frequency mode configuration takes no longer than 45 seconds.

Default settings which may need to be changed as required BEFORE execution of ADF4351 library functions (defaults listed):

Phase Detector Polarity (Register 4/Bit 6 = 1): Negative (passive or noninverting active loop filter)


Error codes:


Common to all of the following subroutines:

ADF4351_ERROR_NONE


SetStepFreq:

ADF4351_ERROR_STEP_FREQUENCY_EXCEEDS_PFD


setf:

ADF4351_ERROR_RF_FREQUENCY

ADF4351_ERROR_POWER_LEVEL

ADF4351_ERROR_AUX_POWER_LEVEL

ADF4351_ERROR_AUX_FREQ_DIVIDER

ADF4351_ERROR_ZERO_PFD_FREQUENCY

ADF4351_ERROR_MOD_RANGE

ADF4351_ERROR_FRAC_RANGE

ADF4351_ERROR_N_RANGE

ADF4351_ERROR_RF_FREQUENCY_AND_STEP_FREQUENCY_HAS_REMAINDER

ADF4351_ERROR_PFD_EXCEEDED_WITH_FRACTIONAL_MODE

ADF4351_ERROR_PRECISION_FREQUENCY_CALCULATION_TIMEOUT


setrf:

ADF4351_ERROR_DOUBLER_EXCEEDED

ADF4351_ERROR_R_RANGE

ADF4351_ERROR_REF_FREQUENCY

ADF4351_ERROR_REF_MULTIPLIER_TYPE


setf and setrf:

ADF4351_ERROR_PFD_AND_STEP_FREQUENCY_HAS_REMAINDER

ADF4351_ERROR_PFD_LIMITS


Warning codes:


setf:

ADF4351_WARNING_FREQUENCY_ERROR

## Installation
Copy the `src/` directory to your Arduino sketchbook directory  (named the directory `example4351`), and install the libraries in your Arduino library directory.  You can also install the ADF4351 files separatly  as a library.

## References

+ [ADF4351 Product Page](https://goo.gl/tkMjw6) Analog Devices
+ [Big Number Arduino Library](https://github.com/nickgammon/BigNumber) by Nick Gammon