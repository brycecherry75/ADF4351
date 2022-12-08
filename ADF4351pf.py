# ADF4351 advanced precision Frequency calculator by Bryce Cherry
# Usage: python adf4351pf.py --ref reference_frequency_in_Hz -rf rf_frequency_in_Hz

import argparse
import math

DesiredFrequency = 0
ReferenceFrequency = 0 # includes selectable reference multiplier or divider (not the R divider)

# ADF4351 limits as per datasheet
MaximumR = 1023
MinimumInt = 23 # under integer mode
MaximumInt = 65535
MaximumMod = 4095
MaximumPFDFrequency = 90000000 # under integer mode with band selection disabled; 45000000 with band selection enabled
MinimumPFDFrequency = 125000
MinimumRFFrequency = 34375000
MaximumRFFrequency = 4400000000
MinimumReferenceFrequency = 10000000
MaximumReferenceFrequency = 250000000

NegativeFrequencyError = False
MatchAttempted = False

# final results
FrequencyError = ReferenceFrequency
MatchingR = 1
MatchingInt = 1
MatchingMod = 2
MatchingFrac = 0
MatchingDivider = 1
MatchingDivider_PowerOf2 = 0
FractionalMode = True
Prescaler = 0

if __name__ == "__main__":
    parser = argparse.ArgumentParser(fromfile_prefix_chars='@')
    parser.add_argument("--ref", type=float, help="reference frequency including doubler/divide by 2 (not from R divider)")
    parser.add_argument("--rf", type=float, help="RF frequency")
    args = parser.parse_args()

    ReferenceFrequency = args.ref
    DesiredFrequency = args.rf

    if ReferenceFrequency >= MinimumReferenceFrequency and ReferenceFrequency <= MaximumReferenceFrequency:
      if (DesiredFrequency > 3600000000):
        Prescaler = 1
        MinimumInt = 75
      if DesiredFrequency >= MinimumRFFrequency and DesiredFrequency <= MaximumRFFrequency:
        while DesiredFrequency < (MaximumRFFrequency / 2): # convert RF frequency to VCO frequency and determine RF division ratio
          MatchingDivider_PowerOf2 += 1
          DesiredFrequency *= 2
        MatchingDivider = 2**MatchingDivider_PowerOf2

        # try integer mode first
        for RtoMatch in range (MaximumR):
          if RtoMatch > 0 and (ReferenceFrequency / RtoMatch) >= MinimumPFDFrequency and (ReferenceFrequency / RtoMatch) <= MaximumPFDFrequency: # do not divide by zero or exceed PFD limits under fractional mode
            IntTemp = (DesiredFrequency / (ReferenceFrequency / RtoMatch))
            if IntTemp >= MinimumInt and IntTemp <= MaximumInt:
              MatchAttempted = True
              if math.remainder(IntTemp, 1) == 0:
                MatchAttempted = True
                FractionalMode = False
                MatchingR = RtoMatch
                MatchingInt = IntTemp
                break
            else:
              break
          else:
            if (RtoMatch > 0):
              break

        # else, try fractional mode
        if FractionalMode == True:
          # change of limits as per datasheet
          MaximumPFDFrequency = 32000000
          for RtoMatch in range (MaximumR):
            if RtoMatch > 0 and (ReferenceFrequency / RtoMatch) >= MinimumPFDFrequency and (ReferenceFrequency / RtoMatch) <= MaximumPFDFrequency: # do not divide by zero or exceed PFD limits under fractional mode
              IntTemp = (DesiredFrequency / (ReferenceFrequency / RtoMatch))
              IntTemp = math.floor(IntTemp)
              if IntTemp >= MinimumInt and IntTemp <= MaximumInt:
                MatchAttempted = True
                FrequencyRemainder = (DesiredFrequency - (IntTemp * (ReferenceFrequency / RtoMatch)))
                for ModToMatch in range (MaximumMod):
                  if ModToMatch >= 2: # minimum MOD is 2 as per datasheet
                    OldFrequencyError = FrequencyError
                    ModFrequencyStep = (ReferenceFrequency / RtoMatch / ModToMatch)
                    FracToMatch = (FrequencyRemainder / ModFrequencyStep) + 0.5
                    FracToMatch = math.floor(FracToMatch)
                    if FracToMatch == ModToMatch:
                      FracToMatch -= 1
                    FrequencyError = (FrequencyRemainder - (ModFrequencyStep * FracToMatch))
                    if FrequencyError < 0: # convert to a positive if necessary
                      FrequencyError *= (-1)
                      NegativeFrequencyError = True
                    else:
                      NegativeFrequencyError = False
                    if FrequencyError < OldFrequencyError:
                      OldFrequencyError = FrequencyError
                      MatchingR = RtoMatch
                      MatchingInt = IntTemp
                      MatchingMod = ModToMatch
                      MatchingFrac = FracToMatch
                    if FrequencyError == 0: # exact frequency can be obtained
                      break
                if FrequencyError == 0: # exact frequency can be obtained
                  break
              else:
                if IntTemp > MaximumInt:
                  break
            else:
              if (RtoMatch > 0):
                break

        if MatchAttempted == True:
          DesiredFrequency /= MatchingDivider # convert VCO frequency to RF frequency
          if FractionalMode == True:
            print ("Fractional mode")
            if NegativeFrequencyError == True:
              FrequencyError *= (-1)
            FrequencyError /= MatchingDivider # convert VCO frequency error to RF frequency error
            print("Frequency error (Hz):", FrequencyError)
            print("Actual frequency (Hz):", (DesiredFrequency + FrequencyError))
          else:
            print ("Integer mode - exact frequency")
          print("R:", MatchingR)
          print("Int:", MatchingInt)
          print("Mod:", MatchingMod)
          print("Frac:", MatchingFrac)
          print("RF divider ratio:", MatchingDivider)
          print("RF divider (power of 2):", MatchingDivider_PowerOf2)
          print("Prescaler:", Prescaler)
        else:
          print("Integer result within datasheet limits is not possible with specified reference and RF frequencies")
      else:
        print("RF frequency is out of range")
    else:
      print("Reference frequency is out of range")