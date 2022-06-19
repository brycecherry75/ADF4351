/*!

   @file ADF4351.cpp

   @mainpage ADF4351 Arduino library driver for Wideband Frequency Synthesizer

   @section intro_sec Introduction

   The ADF4351 chip is a wideband freqency synthesizer integrated circuit that can generate frequencies
   from 34.375 MHz to 4.4 GHz. It incorporates a PLL (Fraction-N and Integer-N modes) and VCO, along with
   prescalers, dividers and multipiers.  The users add a PLL loop filter and reference frequency to
   create a frequency generator with a very wide range, that is tuneable in settable frequency steps.

   The ADF4351 chip provides an SPI interface for setting the device registers that control the
   frequency and output levels, along with several IO pins for gathering chip status and
   enabling/disabling output and power modes.

   The ADF4351 library provides an Arduino API for accessing the features of the ADF chip.

   The basic PLL equations for the ADF4351 are:

   \f$ RF_{out} = f_{PFD} \times (INT +(\frac{FRAC}{MOD})) \f$

   where:

   \f$ f_{PFD} = REF_{IN} \times \left[ \frac{(1 + D)}{( R \times (1 + T))} \right]  \f$

   \f$ D = \textrm{RD2refdouble, ref doubler flag}\f$

   \f$ R = \textrm{RCounter, ref divider}\f$

   \f$ T = \textrm{RD1Rdiv2, ref divide by 2 flag}\f$




   @section dependencies Dependencies

   This library uses the BigNumber library from Nick Gammon
   Requires the BitFieldManipulation library: http://github.com/brycecherry75/BitFieldManipulation
   Requires the BeyondByte library: http://github.com/brycecherry75/BeyondByte

   @section author Author

   Bryce Cherry

*/

#include "ADF4351.h"

ADF4351::ADF4351() {
  SPISettings ADF4351_SPI(10000000UL, MSBFIRST, SPI_MODE0);
}

void ADF4351::WriteRegs() {
  for (int i = 5 ; i >= 0 ; i--) { // sequence according to the ADF4351 datasheet
    SPI.beginTransaction(ADF4351_SPI);
    digitalWrite(ADF4351_PIN_SS, LOW);
    delayMicroseconds(1);
    BeyondByte.writeDword(0, ADF4351_R[i], 4, BeyondByte_SPI, MSBFIRST);
    delayMicroseconds(1);
    digitalWrite(ADF4351_PIN_SS, HIGH);
    SPI.endTransaction();
    delayMicroseconds(1);
  }
}

void ADF4351::WriteSweepValues(const uint32_t *regs) {
  for (int i = 0; i < ADF4351_RegsToWrite; i++) {
    ADF4351_R[i] = regs[i];
  }
  WriteRegs();
}

void ADF4351::ReadSweepValues(uint32_t *regs) {
  for (int i = 0; i < ADF4351_RegsToWrite; i++) {
    regs[i] = ADF4351_R[i];
  }
}

uint16_t ADF4351::ReadR() {
  return BitFieldManipulation.ReadBF_dword(14, 10, ADF4351_R[0x02]);
}

uint16_t ADF4351::ReadInt() {
  return BitFieldManipulation.ReadBF_dword(15, 16, ADF4351_R[0x00]);
}

uint16_t ADF4351::ReadFraction() {
  return BitFieldManipulation.ReadBF_dword(3, 12, ADF4351_R[0x00]);
}

uint16_t ADF4351::ReadMod() {
  return BitFieldManipulation.ReadBF_dword(3, 12, ADF4351_R[0x01]);
}

uint8_t ADF4351::ReadOutDivider() {
  return (1 << BitFieldManipulation.ReadBF_dword(20, 3, ADF4351_R[0x04]));
}

uint8_t ADF4351::ReadOutDivider_PowerOf2() {
  return BitFieldManipulation.ReadBF_dword(20, 3, ADF4351_R[0x04]);
}

uint8_t ADF4351::ReadRDIV2() {
  return BitFieldManipulation.ReadBF_dword(24, 1, ADF4351_R[0x02]);
}

uint8_t ADF4351::ReadRefDoubler() {
  return BitFieldManipulation.ReadBF_dword(25, 1, ADF4351_R[0x02]);
}

int32_t ADF4351::ReadFrequencyError() {
  return ADF4351_FrequencyError;
}

double ADF4351::ReadPFDfreq() {
  double value = ADF4351_reffreq;
  uint16_t temp = ReadR();
  if (temp == 0) { // avoid division by zero
    return 0;
  }
  value /= temp;
  if (ReadRDIV2() != 0) {
    value /= 2;
  }
  if (ReadRefDoubler() != 0) {
    value *= 2;
  }
  return value;
}

void ADF4351::ReadCurrentFrequency(char *freq)
{
  BigNumber::begin(12);
  char tmpstr[12];
  ultoa(ADF4351_reffreq, tmpstr, 10);
  BigNumber BN_ref = BigNumber(tmpstr);
  if (ReadRDIV2() != 0 && ReadRefDoubler() == 0) {
    BN_ref /= BigNumber(2);
  }
  else if (ReadRDIV2() == 0 && ReadRefDoubler() != 0) {
    BN_ref *= BigNumber(2);
  }
  BN_ref /= BigNumber(ReadR());
  BigNumber BN_freq = BN_ref;
  BN_freq *= BigNumber(ReadInt());
  BN_ref *= BigNumber(ReadFraction());
  BN_ref /= BigNumber(ReadMod());
  BN_freq += BN_ref;
  BN_freq /= BigNumber(ReadOutDivider());
  BigNumber BN_rounding = BigNumber("0.5");
  for (int i = 0; i < ADF4351_DECIMAL_PLACES; i++) {
    BN_rounding /= BigNumber(10);
  }
  BN_freq += BN_rounding;
  char* temp = BN_freq.toString();
  BigNumber::finish();
  uint8_t DecimalPlaceToStart;
  for (int i = 0; i < (ADF4351_DIGITS + 1); i++){
    freq[i] = temp[i];
    if (temp[i] == '.') {
      DecimalPlaceToStart = i;
      DecimalPlaceToStart++;
      break;
    }
  }
  for (int i = DecimalPlaceToStart; i < (DecimalPlaceToStart + ADF4351_DECIMAL_PLACES); i++) {
    freq[i] = temp[i];
  }
  freq[(DecimalPlaceToStart + ADF4351_DECIMAL_PLACES)] = 0x00;
  free(temp);
}

void ADF4351::init(uint8_t SSpin, uint8_t LockPinNumber, bool Lock_Pin_Used, uint8_t CEpinNumber, bool CE_Pin_Used)
{
  ADF4351_PIN_SS = SSpin;
  pinMode(ADF4351_PIN_SS, OUTPUT) ;
  digitalWrite(ADF4351_PIN_SS, HIGH) ;
  if (CE_Pin_Used == true) {
    pinMode(CEpinNumber, OUTPUT) ;
  }
  if (Lock_Pin_Used == true) {
    pinMode(LockPinNumber, INPUT_PULLUP) ;
  }
  SPI.begin();
}

int ADF4351::SetStepFreq(uint32_t value) {
  if (value > ReadPFDfreq()) {
    return ADF4351_ERROR_STEP_FREQUENCY_EXCEEDS_PFD;
  }
  uint32_t ReferenceFrequency = ADF4351_reffreq;
  uint16_t Rvalue = ReadR();
  if (Rvalue == 0 || (ADF4351_reffreq % value) != 0) {
    return ADF4351_ERROR_PFD_AND_STEP_FREQUENCY_HAS_REMAINDER;
  }
  ADF4351_ChanStep = value;
  return ADF4351_ERROR_NONE;
}

int ADF4351::setf(char *freq, uint8_t PowerLevel, uint8_t AuxPowerLevel, uint8_t AuxFrequencyDivider, bool PrecisionFrequency, uint32_t MaximumFrequencyError, uint32_t CalculationTimeout) {
  ADF4351_FrequencyError = 0;
  //  calculate settings from freq
  if (PowerLevel < 0 || PowerLevel > 4) return ADF4351_ERROR_POWER_LEVEL;
  if (AuxPowerLevel < 0 || AuxPowerLevel > 4) return ADF4351_ERROR_AUX_POWER_LEVEL;
  if (AuxFrequencyDivider != ADF4351_AUX_DIVIDED && AuxFrequencyDivider != ADF4351_AUX_FUNDAMENTAL) return ADF4351_ERROR_AUX_FREQ_DIVIDER;
  if (ReadPFDfreq() == 0) return ADF4351_ERROR_ZERO_PFD_FREQUENCY;

  uint32_t ReferenceFrequency = ADF4351_reffreq;
  ReferenceFrequency /= ReadR();
  if (PrecisionFrequency == false && ADF4351_ChanStep > 1 && (ReferenceFrequency % ADF4351_ChanStep) != 0) {
    return ADF4351_ERROR_PFD_AND_STEP_FREQUENCY_HAS_REMAINDER;
  }

  BigNumber::begin(12); // for a maximum 90 MHz PFD and a 64 RF divider with frequency steps no smaller than 1 Hz, will fit the maximum of 5.76 * (10 ^ 9) for the MOD and FRAC before GCD calculation

  if (BigNumber(freq) > BigNumber("4400000000") || BigNumber(freq) < BigNumber("34375000")) {
    BigNumber::finish();
    return ADF4351_ERROR_RF_FREQUENCY;
  }

  uint8_t FrequencyPointer = 0;
  while (true) { // null out any decimal places below 1 Hz increments to avoid GCD calculation input overflow
    if (freq[FrequencyPointer] == '.') { // change the decimal point to a null terminator
      freq[FrequencyPointer] = 0x00;
      break;
    }
    if (freq[FrequencyPointer] == 0x00) { // null terminator reached
      break;
    }
    FrequencyPointer++;
  }

  char tmpstr[12]; // will fit a long including sign and terminator

  if (PrecisionFrequency == false && ADF4351_ChanStep > 1) {
    ultoa(ADF4351_ChanStep, tmpstr, 10);
    BigNumber BN_freq = BigNumber(freq);
    // BigNumber has issues with modulus calculation which always results in 0
    BN_freq /= BigNumber(tmpstr);
    uint32_t ChanSteps = (uint32_t)((uint32_t) BN_freq); // round off the decimal - overflow is not an issue for the ADF4351 frequency range
    ultoa(ChanSteps, tmpstr, 10);
    BN_freq -= BigNumber(tmpstr);
    if (BN_freq != BigNumber(0)) {
      BigNumber::finish();
      return ADF4351_ERROR_RF_FREQUENCY_AND_STEP_FREQUENCY_HAS_REMAINDER;
    }
  }

  BigNumber BN_localosc_ratio = BigNumber("2200000000") / BigNumber(freq);
  uint8_t localosc_ratio = (uint32_t)((uint32_t) BN_localosc_ratio);
  uint8_t ADF4351_outdiv = 1;
  uint8_t ADF4351_RfDivSel = 0;
  uint8_t ADF4351_Prescaler = 0;
  uint32_t ADF4351_N_Int;
  uint32_t ADF4351_Mod = 2;
  uint32_t ADF4351_Frac = 0;

  ultoa(ADF4351_reffreq, tmpstr, 10);
  word CurrentR = ReadR();
  uint8_t RDIV2 = ReadRDIV2();
  uint8_t RefDoubler = ReadRefDoubler();
  BigNumber BN_ADF4351_PFDFreq = (BigNumber(tmpstr) * (BigNumber(1) * BigNumber((1 + RefDoubler))) * (BigNumber(1) / BigNumber((1 + RDIV2)))) / BigNumber(CurrentR);
  uint32_t PFDFreq = (uint32_t)((uint32_t) BN_ADF4351_PFDFreq); // used for checking maximum PFD limit under Fractional Mode

  // select the output divider
  if (BigNumber(freq) > BigNumber("34375000")) {
    while (ADF4351_outdiv <= localosc_ratio && ADF4351_outdiv <= 64) {
      ADF4351_outdiv *= 2;
      ADF4351_RfDivSel++;
    }
  }
  else {
    ADF4351_outdiv = 64;
    ADF4351_RfDivSel = 6;
  }

  if (BigNumber(freq) > BigNumber("3600000000")) {
    ADF4351_Prescaler = 1;
  }

  bool CalculationTookTooLong = false;
  BigNumber BN_ADF4351_N_Int = (BigNumber(freq) / BN_ADF4351_PFDFreq) * BigNumber(ADF4351_outdiv); // for 4007.5 MHz RF/10 MHz PFD, result is 400.75;
  ADF4351_N_Int = (uint32_t)((uint32_t) BN_ADF4351_N_Int); // round off the decimal
  ultoa(ADF4351_N_Int, tmpstr, 10);
  BigNumber BN_FrequencyRemainder;
  if (PrecisionFrequency == true) { // frequency is 4007.5 MHz, PFD is 10 MHz and output divider is 2
    uint32_t CalculationTimeStart = millis();
    BN_FrequencyRemainder = ((BN_ADF4351_PFDFreq * BigNumber(tmpstr)) / BigNumber(ADF4351_outdiv)) - BigNumber(freq); // integer is 4000 MHz, remainder is -7.5 MHz and will be converterd to a positive
    if (BN_FrequencyRemainder < BigNumber(0)) {
      BN_FrequencyRemainder *= BigNumber("-1"); // convert to a postivie
    }
    BigNumber BN_ADF4351_N_Int_Overflow = (BN_ADF4351_N_Int + BigNumber("0.00024421")); // deal with N having remainder greater than (4094 / 4095) and a frequency within ((PFD - (PFD * (1 / 4095)) / output divider)
    uint32_t ADF4351_N_Int_Overflow = (uint32_t)((uint32_t) BN_ADF4351_N_Int_Overflow);
    if (ADF4351_N_Int_Overflow == ADF4351_N_Int) { // deal with N having remainder greater than (4094 / 4095) and a frequency within ((PFD - (PFD * (1 / 4095)) / output divider)
      ADF4351_FrequencyError = (int32_t)((int32_t) BN_FrequencyRemainder); // initial value should the MOD match loop fail to result in FRAC < MOD
      if (ADF4351_FrequencyError > MaximumFrequencyError) { // use fractional division if out of tolerance
        uint32_t FreqeucnyError = ADF4351_FrequencyError;
        uint32_t PreviousFrequencyError = ADF4351_FrequencyError;
        for (word ModToMatch = 2; ModToMatch <= 4095; ModToMatch++) {
          if (CalculationTimeout > 0) {
            uint32_t CalculationTime = millis();
            CalculationTime -= CalculationTimeStart;
            if (CalculationTime > CalculationTimeout) {
              CalculationTookTooLong = true;
              break;
            }
          }
          BigNumber BN_ModFrequencyStep = BN_ADF4351_PFDFreq / BigNumber(ModToMatch) / BigNumber(ADF4351_outdiv); // For 4007.5 MHz RF/10 MHz PFD, should be 4
          BigNumber BN_TempFrac = (BN_FrequencyRemainder / BN_ModFrequencyStep) + BigNumber("0.5"); // result should be 3 to correspond with above line
          uint32_t TempFrac = (uint32_t)((uint32_t) BN_TempFrac);
          if (TempFrac <= ModToMatch) { // FRAC must be < MOD
            if (TempFrac == ModToMatch) { // FRAC must be < MOD
              TempFrac--;
            }
            ultoa(TempFrac, tmpstr, 10);
            BigNumber BN_FrequencyError = (BN_FrequencyRemainder - (BigNumber(tmpstr) * BN_ModFrequencyStep));
            if (BN_FrequencyError < BigNumber(0)) {
              BN_FrequencyError *= BigNumber("-1"); // convert to a positive
            }
            ADF4351_FrequencyError = (int32_t)((int32_t) BN_FrequencyError);
            if (ADF4351_FrequencyError < PreviousFrequencyError) {
              PreviousFrequencyError = ADF4351_FrequencyError;
              ADF4351_Mod = ModToMatch; // result should be 4 for 4007.5 MHz/10 MHz PFD
              ADF4351_Frac = TempFrac; // result should be 3 to correspond with above line
            }
            if (ADF4351_FrequencyError <= MaximumFrequencyError) { // tolerance has been obtained - for 4007.5 MHz, MOD = 4, FRAC = 3; error = 0
              break;
            }
          }
        }
      }
    }
    else {
      ADF4351_N_Int++;
    }
    ultoa(ADF4351_N_Int, tmpstr, 10);
  }
  else {
    BN_ADF4351_N_Int = (((BigNumber(freq) * BigNumber(ADF4351_outdiv))) / BN_ADF4351_PFDFreq);
    ADF4351_N_Int = (uint32_t)((uint32_t) BN_ADF4351_N_Int);
    ultoa(ADF4351_ChanStep, tmpstr, 10);
    BigNumber BN_ADF4351_Mod = (BN_ADF4351_PFDFreq / (BigNumber(tmpstr) / BigNumber(ADF4351_outdiv)));
    ultoa(ADF4351_N_Int, tmpstr, 10);
    BigNumber BN_ADF4351_Frac = (((BN_ADF4351_N_Int - BigNumber(tmpstr)) * BN_ADF4351_Mod) + BigNumber("0.5"));
    // for a maximum 90 MHz PFD and a 64 RF divider with frequency steps no smaller than 1 Hz, maximum results for each is 5.76 * (10 ^ 9) but can be divided by the RF division ratio without error (results will be no larger than 90 * (10 ^ 6))
    BN_ADF4351_Frac /= BigNumber(ADF4351_outdiv);
    BN_ADF4351_Mod /= BigNumber(ADF4351_outdiv);

    // calculate the GCD - Mod2/Frac2 values are temporary
    uint32_t GCD_ADF4351_Mod2 = (uint32_t)((uint32_t) BN_ADF4351_Mod);
    uint32_t GCD_ADF4351_Frac2 = (uint32_t)((uint32_t) BN_ADF4351_Frac);
    uint32_t GCD_t;
    while (true) {
      if (GCD_ADF4351_Mod2 == 0) {
        GCD_t = GCD_ADF4351_Frac2;
        break;
      }
      if (GCD_ADF4351_Frac2 == 0) {
        GCD_t = GCD_ADF4351_Mod2;
        break;
      }
      if (GCD_ADF4351_Mod2 == GCD_ADF4351_Frac2) {
        GCD_t = GCD_ADF4351_Mod2;
        break;
      }
      if (GCD_ADF4351_Mod2 > GCD_ADF4351_Frac2) {
        GCD_ADF4351_Mod2 -= GCD_ADF4351_Frac2;
      }
      else {
        GCD_ADF4351_Frac2 -= GCD_ADF4351_Mod2;
      }
    }
    // restore the original Mod2/Frac2 temporary values before dividing by GCD
    GCD_ADF4351_Mod2 = (uint32_t)((uint32_t) BN_ADF4351_Mod);
    GCD_ADF4351_Frac2 = (uint32_t)((uint32_t) BN_ADF4351_Frac);
    GCD_ADF4351_Mod2 /= GCD_t;
    GCD_ADF4351_Frac2 /= GCD_t;
    if (GCD_ADF4351_Mod2 > 4095) { // outside valid range
      while (true) {
        GCD_ADF4351_Mod2 /= 2;
        GCD_ADF4351_Frac2 /= 2;
        if (GCD_ADF4351_Mod2 <= 4095) { // now within valid range
          if (GCD_ADF4351_Frac2 == GCD_ADF4351_Mod2) { // FRAC must be less than MOD
            GCD_ADF4351_Frac2--;
          }
          break;
        }
      }
    }
    // set the final FRAC/MOD values
    ADF4351_Frac = GCD_ADF4351_Frac2;
    ADF4351_Mod = GCD_ADF4351_Mod2;
  }

  if (CalculationTookTooLong == true) {
    BigNumber::finish();
    return ADF4351_ERROR_PRECISION_FREQUENCY_CALCULATION_TIMEOUT;
  }

  // tmpstr is the N value
  BN_FrequencyRemainder = (((((BN_ADF4351_PFDFreq * BigNumber(tmpstr)) + (BigNumber(ADF4351_Frac) * (BN_ADF4351_PFDFreq / BigNumber(ADF4351_Mod)))) / BigNumber(ADF4351_outdiv))) - BigNumber(freq)) + BigNumber("0.5"); // no issue with divide by 0 regarding MOD (set to 2 by default) and FRAC (set to 0 by default) - maximum is PFD maximum frequency of 90 MHz under integer mode - no issues with signed overflow or underflow
  ADF4351_FrequencyError = (int32_t)((int32_t) BN_FrequencyRemainder);

  BigNumber::finish();

  if (ADF4351_Frac == 0) { // correct the MOD to the minimum required value
    ADF4351_Mod = 2;
  }

  if (ADF4351_Mod < 2 || ADF4351_Mod > 4095) {
    return ADF4351_ERROR_MOD_RANGE;
  }
  if (ADF4351_Frac > (ADF4351_Mod - 1) ) {
    return ADF4351_ERROR_FRAC_RANGE;
  }
  if (ADF4351_Prescaler == 0 && (ADF4351_N_Int < 23  || ADF4351_N_Int > 65535)) {
    return ADF4351_ERROR_N_RANGE;
  }
  if (ADF4351_Prescaler == 1 && (ADF4351_N_Int < 75 || ADF4351_N_Int > 65535)) {
    return ADF4351_ERROR_N_RANGE_OVER_3600_MHz;
  }
  if (ADF4351_Frac != 0 && PFDFreq > ADF4351_PFD_MAX_FRAC) {
    return ADF4351_ERROR_PFD_EXCEEDED_WITH_FRACTIONAL_MODE;
  }

  ADF4351_R[0x00] = BitFieldManipulation.WriteBF_dword(3, 12, ADF4351_R[0x00], ADF4351_Frac);
  ADF4351_R[0x00] = BitFieldManipulation.WriteBF_dword(15, 16, ADF4351_R[0x00], ADF4351_N_Int);
  ADF4351_R[0x01] = BitFieldManipulation.WriteBF_dword(3, 12, ADF4351_R[0x01], ADF4351_Mod);
  ADF4351_R[0x01] = BitFieldManipulation.WriteBF_dword(27, 1, ADF4351_R[0x01], ADF4351_Prescaler);
  // (0x01, 28,1,0) phase adjust
  // (0x02, 3,1,0) counter reset
  // (0x02, 4,1,0) cp3 state
  // (0x02, 5,1,0) power down

  if ( ADF4351_Frac == 0 )  {
    ADF4351_R[0x02] = BitFieldManipulation.WriteBF_dword(7, 1, ADF4351_R[0x02], 1); // LDP, int-n mode
    ADF4351_R[0x02] = BitFieldManipulation.WriteBF_dword(8, 1, ADF4351_R[0x02], 1); // ldf, int-n mode
    if (PFDFreq > 45000000UL) { // ref ADF4351 Datasheet: Phase Freqeuncy Detector (PFD) and Charge Pump
      ADF4351_R[0x01] = BitFieldManipulation.WriteBF_dword(28, 1, ADF4351_R[0x01], 1);
    }
    else {
      ADF4351_R[0x01] = BitFieldManipulation.WriteBF_dword(28, 1, ADF4351_R[0x01], 0);
    }
  }
  else {
    ADF4351_R[0x01] = BitFieldManipulation.WriteBF_dword(28, 1, ADF4351_R[0x01], 0);// ref ADF4351 Datasheet: Phase Freqeuncy Detector (PFD) and Charge Pump
    ADF4351_R[0x02] = BitFieldManipulation.WriteBF_dword(7, 1, ADF4351_R[0x02], 0); // LDP, int-n mode
    ADF4351_R[0x02] = BitFieldManipulation.WriteBF_dword(8, 1, ADF4351_R[0x02], 0); // ldf, int-n mode
  }
  // (0x02, 13,1,0) dbl buf
  // (0x02, 26,3,0) //  muxout, not used
  // (0x02, 29,2,0) low noise and spurs mode
  // (0x03, 15,2,0) clk div mode
  // (0x03, 17,1,0) reserved
  // (0x03, 18,1,0) CSR
  // (0x03, 19,2,0) reserved
  if ( ADF4351_Frac == 0 )  {
    ADF4351_R[0x03] = BitFieldManipulation.WriteBF_dword(21, 1, ADF4351_R[0x03], 1); //  charge cancel, reduces pfd spurs
    ADF4351_R[0x03] = BitFieldManipulation.WriteBF_dword(22, 1, ADF4351_R[0x03], 1); //  ABP, int-n
  } else  {
    ADF4351_R[0x03] = BitFieldManipulation.WriteBF_dword(21, 1, ADF4351_R[0x03], 0); //  charge cancel
    ADF4351_R[0x03] = BitFieldManipulation.WriteBF_dword(22, 1, ADF4351_R[0x03], 0); //  ABP, frac-n
  }
  // (0x03, 24,8,0) reserved
  if (PowerLevel == 0) {
    ADF4351_R[0x04] = BitFieldManipulation.WriteBF_dword(5, 1, ADF4351_R[0x04], 0);
  }
  else {
    PowerLevel--;
    ADF4351_R[0x04] = BitFieldManipulation.WriteBF_dword(5, 1, ADF4351_R[0x04], 1);
    ADF4351_R[0x04] = BitFieldManipulation.WriteBF_dword(3, 2, ADF4351_R[0x04], PowerLevel);
  }
  if (AuxPowerLevel == 0) {
    ADF4351_R[0x04] = BitFieldManipulation.WriteBF_dword(8, 1, ADF4351_R[0x04], 0);
  }
  else {
    AuxPowerLevel--;
    ADF4351_R[0x04] = BitFieldManipulation.WriteBF_dword(6, 2, ADF4351_R[0x04], AuxPowerLevel);
    ADF4351_R[0x04] = BitFieldManipulation.WriteBF_dword(8, 1, ADF4351_R[0x04], 1);
    ADF4351_R[0x04] = BitFieldManipulation.WriteBF_dword(9, 1, ADF4351_R[0x04], AuxFrequencyDivider);
  }
  // (0x04, 10,1,0) mtld
  // (0x04, 11,1,0) vco power down
  ADF4351_R[0x04] = BitFieldManipulation.WriteBF_dword(20, 3, ADF4351_R[0x04], ADF4351_RfDivSel);
  // (0x04, 24,8,0) reserved
  WriteRegs();

  bool NegativeError = false;
  if (ADF4351_FrequencyError < 0) { // convert to a positive for frequency error comparison with a positive value
    ADF4351_FrequencyError ^= 0xFFFFFFFF;
    ADF4351_FrequencyError++;
    NegativeError = true;
  }
  if ((PrecisionFrequency == true && ADF4351_FrequencyError > MaximumFrequencyError) || (PrecisionFrequency == false && ADF4351_FrequencyError != 0)) {
    if (NegativeError == true) { // convert back to negative if changed from negative to positive for frequency error comparison with a positive value
      ADF4351_FrequencyError ^= 0xFFFFFFFF;
      ADF4351_FrequencyError++;
    }
    return ADF4351_WARNING_FREQUENCY_ERROR;
  }
  return ADF4351_ERROR_NONE; // ok
}

int ADF4351::setrf(uint32_t f, uint16_t r, uint8_t ReferenceDivisionType) {
  if (r > 1023 || r < 1) return ADF4351_ERROR_R_RANGE;
  if (f < ADF4351_REFIN_MIN || f > ADF4351_REFIN_MAX) return ADF4351_ERROR_REF_FREQUENCY;
  if (ReferenceDivisionType != ADF4351_REF_UNDIVIDED && ReferenceDivisionType != ADF4351_REF_HALF && ReferenceDivisionType != ADF4351_REF_DOUBLE) return ADF4351_ERROR_REF_MULTIPLIER_TYPE;
  if (f > 30000000UL && ReferenceDivisionType == ADF4351_REF_DOUBLE) return ADF4351_ERROR_DOUBLER_EXCEEDED;

  double ReferenceFactor = 1;
  if (ReferenceDivisionType == ADF4351_REF_HALF) {
    ReferenceFactor /= 2;
  }
  else if (ReferenceDivisionType == ADF4351_REF_DOUBLE) {
    ReferenceFactor *= 2;
  }
  double newfreq  =  (double) f  * ( (double) ReferenceFactor / (double) r);  // check the loop freq

  if ( newfreq > ADF4351_PFD_MAX || newfreq < ADF4351_PFD_MIN ) return ADF4351_ERROR_PFD_LIMITS;

  ADF4351_reffreq = f ;
  ADF4351_R[0x02] = BitFieldManipulation.WriteBF_dword(14, 10, ADF4351_R[0x02], r);
  if (ReferenceDivisionType == ADF4351_REF_DOUBLE) {
    ADF4351_R[0x02] = BitFieldManipulation.WriteBF_dword(24, 2, ADF4351_R[0x02], 0b00000010);
  }
  else if (ReferenceDivisionType == ADF4351_REF_HALF) {
    ADF4351_R[0x02] = BitFieldManipulation.WriteBF_dword(24, 2, ADF4351_R[0x02], 0b00000001);
  }
  else {
    ADF4351_R[0x02] = BitFieldManipulation.WriteBF_dword(24, 2, ADF4351_R[0x02], 0b00000000);
  }
  return ADF4351_ERROR_NONE;
}