/*!
   @file ADF4351.h

   This is part of the Arduino Library for the ADF4351 PLL wideband frequency synthesier

*/

#ifndef ADF4351_H
#define ADF4351_H
#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>
#include <BigNumber.h>
#include <BitFieldManipulation.h>
#include <BeyondByte.h>

#define ADF4351_PFD_MAX  45000000UL      ///< Maximum Frequency for Phase Detector under Integer Mode with VCO band selection enabled (Bit 28 of ADF4351_R[1] = 0)
#define ADF4351_PFD_MAX_FRAC   32000000UL      ///< Maximum Frequency for Phase Detector under Fractional Mode
#define ADF4351_PFD_MIN   125000UL        ///< Minimum Frequency for Phase Detector
#define ADF4351_REFIN_MIN   100000UL      ///< Minimum Reference Frequency
#define ADF4351_REFIN_MAX   250000000UL   ///< Maximum Reference Frequency
#define ADF4351_REF_FREQ_DEFAULT 10000000UL  ///< Default Reference Frequency

#define ADF4351_AUX_DIVIDED 0
#define ADF4351_AUX_FUNDAMENTAL 1
#define ADF4351_REF_UNDIVIDED 0
#define ADF4351_REF_HALF 1
#define ADF4351_REF_DOUBLE 2

// common to all of the following subroutines
#define ADF4351_ERROR_NONE 0

// SetStepFreq
#define ADF4351_ERROR_STEP_FREQUENCY_EXCEEDS_PFD 1

// setf
#define ADF4351_ERROR_RF_FREQUENCY 2
#define ADF4351_ERROR_POWER_LEVEL 3
#define ADF4351_ERROR_AUX_POWER_LEVEL 4
#define ADF4351_ERROR_AUX_FREQ_DIVIDER 5
#define ADF4351_ERROR_ZERO_PFD_FREQUENCY 6
#define ADF4351_ERROR_MOD_RANGE 7
#define ADF4351_ERROR_FRAC_RANGE 8
#define ADF4351_ERROR_N_RANGE 9
#define ADF4351_ERROR_N_RANGE_OVER_3600_MHz 10
#define ADF4351_ERROR_RF_FREQUENCY_AND_STEP_FREQUENCY_HAS_REMAINDER 11
#define ADF4351_ERROR_PFD_EXCEEDED_WITH_FRACTIONAL_MODE 12
#define ADF4351_ERROR_PRECISION_FREQUENCY_CALCULATION_TIMEOUT 13
#define ADF4351_WARNING_FREQUENCY_ERROR 14

// setrf
#define ADF4351_ERROR_DOUBLER_EXCEEDED 15
#define ADF4351_ERROR_R_RANGE 16
#define ADF4351_ERROR_REF_FREQUENCY 17
#define ADF4351_ERROR_REF_MULTIPLIER_TYPE 18

// setf and setrf
#define ADF4351_ERROR_PFD_AND_STEP_FREQUENCY_HAS_REMAINDER 19
#define ADF4351_ERROR_PFD_LIMITS 20

#define ADF4351_RegsToWrite 5UL // for high speed sweep

// ReadCurrentFrequency
#define ADF4351_DIGITS 10
#define ADF4351_DECIMAL_PLACES 6
#define ADF4351_ReadCurrentFrequency_ArraySize (ADF4351_DIGITS + ADF4351_DECIMAL_PLACES + 2) // including decimal point and null terminator

/*!
   @brief ADF4351 chip device driver

   This class provides the overall interface for ADF4351 chip. It is used
   to define the SPI connection, initalize the chip on power up, disable/enable
   frequency generation, and set the frequency and reference frequency.

   The PLL values and register values can also be set directly with this class,
   and current settings for the chip and PLL can be read.

   As a simple frequency generator, once the target frequency and desired channel step
   value is set, the library will perform the required calculations to
   set the PLL and other values, and determine the mode (Frac-N or Int-N)
   for the PLL loop. This greatly simplifies the use of the ADF4351 chip.

   The ADF4351 datasheet should be consulted to understand the correct
   register settings. While a number of checks are provided in the library,
   not all values are checked for allowed settings, so YMMV.

*/
class ADF4351
{
  public:
    /*!
       Constructor
       creates an object and sets the SPI parameters.
       see the Arduino SPI library for the parameter values.
       @param pin the SPI Slave Select Pin to use
       @param mode the SPI Mode (see SPI mode define values)
       @param speed the SPI Serial Speed (see SPI speed values)
       @param order the SPI bit order (see SPI bit order values)
    */
    uint8_t ADF4351_PIN_SS = 10;   ///< Ard Pin for SPI Slave Select

    ADF4351();
    void WriteRegs();

    uint16_t ReadR();
    uint16_t ReadInt();
    uint16_t ReadFraction();
    uint16_t ReadMod();
    uint8_t ReadOutDivider();
    uint8_t ReadOutDivider_PowerOf2();
    uint8_t ReadRDIV2();
    uint8_t ReadRefDoubler();
    double ReadPFDfreq();
    int32_t ReadFrequencyError();

    void init(uint8_t SSpin, uint8_t LockPinNumber, bool Lock_Pin_Used, uint8_t CEpin, bool CE_Pin_Used) ;
    int SetStepFreq(uint32_t value);
    int setf(char *freq, uint8_t PowerLevel, uint8_t AuxPowerLevel, uint8_t AuxFrequencyDivider, bool PrecisionFrequency, uint32_t FrequencyTolerance, uint32_t CalculationTimeout) ; // set freq and power levels and output mode with option for precision frequency setting with tolerance in Hz
    int setrf(uint32_t f, uint16_t r, uint8_t ReferenceDivisionType) ; // set reference freq and reference divider (default is 10 MHz with divide by 1)
    int setPowerLevel(uint8_t PowerLevel);
    int setAuxPowerLevel(uint8_t PowerLevel);

    void WriteSweepValues(const uint32_t *regs);
    void ReadSweepValues(uint32_t *regs);
    void ReadCurrentFrequency(char *freq);

    SPISettings ADF4351_SPI;

    int32_t ADF4351_FrequencyError = 0;
    // power on defaults
    uint32_t ADF4351_reffreq = ADF4351_REF_FREQ_DEFAULT;
    uint32_t ADF4351_R[6] {0x00000000, 0x00008011, 0x00006FC2, 0x00E00483, 0x00850004, 0x00580005};
    uint32_t ADF4351_ChanStep = 100000UL;

};

#endif