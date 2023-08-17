# ADF4351 super precision frequency calculator by Bryce Cherry
# Usage: python adf4351spf.py -rf rf_frequency_in_Hz_float --refstart reference_frequency_in_Hz_int --steps Hz_step_iterations_int

import argparse
import math

DesiredFrequency = 0
ReferenceFrequency = 0 # includes selectable reference multiplier or divider (not the R divider)

# ADF4351 limits as per datasheet
MaximumR = 1023
MinimumInt = 23 # under integer mode
MaximumInt = 65535
MaximumMod = 4095
MaximumPFDFrequency = 45000000 # under integer mode with band selection enabled; 90000000 with band selection disabled
MinimumPFDFrequency = 125000
MinimumRFFrequency = 34375000
MaximumRFFrequency = 4400000000
MinimumReferenceFrequency = 10000000
MaximumReferenceFrequency = 250000000

NegativeFrequencyError = False
MatchAttempted = False
IntegerMode = True

# final results
FrequencyError = MaximumReferenceFrequency
ReferenceFrequencyToMatch = 0
MatchingR = 1
MatchingInt = 1
MatchingMod = 2
MatchingFrac = 0
MatchingDivider = 1
MatchingDivider_PowerOf2 = 0
Prescaler = 0

if __name__ == "__main__":
  parser = argparse.ArgumentParser(fromfile_prefix_chars='@')
  parser.add_argument("--rf", type=float, help="RF frequency")
  parser.add_argument("--refstart", type=int, help="Reference frequency start")
  parser.add_argument("--steps", type=int, help="Reference frequency 1 Hz step iterations")
  args = parser.parse_args()

  DesiredFrequency = args.rf
  ReferenceFrequencyStart = args.refstart
  ReferenceSteps = args.steps
  FinalReferenceFrequency = 0

  if (ReferenceFrequencyStart + ReferenceSteps) > MaximumReferenceFrequency:
    ReferenceFrequencyStart = (MaximumReferenceFrequency - ReferenceSteps)
    print("Changing start reference frequency to", ReferenceFrequencyStart, "Hz")
  if (ReferenceFrequencyStart < MinimumReferenceFrequency):
    ReferenceFrequencyStart = MinimumReferenceFrequency
    print("Changing start reference frequency to", ReferenceFrequencyStart, "Hz")

  for ReferenceFrequencyStepToMatch in range (ReferenceSteps + 1):
      ReferenceFrequencyToMatch = ReferenceFrequencyStart + ReferenceFrequencyStepToMatch
      if (ReferenceFrequencyStepToMatch % 10) == 0:
        print(ReferenceFrequencyStepToMatch)
      FractionalMode = True
      if (DesiredFrequency > 3600000000):
        Prescaler = 1
        MinimumInt = 75
      if DesiredFrequency >= MinimumRFFrequency and DesiredFrequency <= MaximumRFFrequency:
        while DesiredFrequency < (MaximumRFFrequency / 2): # convert RF frequency to VCO frequency and determine RF division ratio
          MatchingDivider_PowerOf2 += 1
          DesiredFrequency *= 2
        MatchingDivider = 2**MatchingDivider_PowerOf2
      else:
        print("RF frequency is out of range")
        break

      # try integer mode first
      if IntegerMode == True:
        MaximumPFDFrequency = 90000000
        for RtoMatch in range (MaximumR + 1):
          if RtoMatch > 0 and (ReferenceFrequencyToMatch / RtoMatch) >= MinimumPFDFrequency and (ReferenceFrequencyToMatch / RtoMatch) <= MaximumPFDFrequency: # do not divide by zero or exceed PFD limits under fractional mode
            IntTemp = (DesiredFrequency / (ReferenceFrequencyToMatch / RtoMatch))
            if int(IntTemp) >= MinimumInt and int(IntTemp) <= MaximumInt:
              MatchAttempted = True
              if math.remainder(IntTemp, 1) == 0:
                FinalReferenceFrequency = ReferenceFrequencyToMatch
                MatchAttempted = True
                FractionalMode = False
                MatchingR = RtoMatch
                MatchingInt = IntTemp
                MatchingMod = 2
                MatchingFrac = 0
                break
            else:
              break
        if FractionalMode == False:
          break

      # else, try fractional mode
      if FractionalMode == True:
        IntegerMode = False
        # change of limits as per datasheet
        MaximumPFDFrequency = 32000000
        for RtoMatch in range (MaximumR + 1):
          if RtoMatch > 0 and (ReferenceFrequencyToMatch / RtoMatch) >= MinimumPFDFrequency and (ReferenceFrequencyToMatch / RtoMatch) <= MaximumPFDFrequency: # do not divide by zero or exceed PFD limits under fractional mode
            IntTemp = (DesiredFrequency / (ReferenceFrequencyToMatch / RtoMatch))
            IntTemp = math.floor(IntTemp)
            if IntTemp >= MinimumInt and IntTemp <= MaximumInt:
              MatchAttempted = True
              FrequencyRemainder = (DesiredFrequency - (IntTemp * (ReferenceFrequencyToMatch / RtoMatch)))
              for ModToMatch in range (MaximumMod + 1):
                if ModToMatch >= 2: # minimum MOD is 2 as per datasheet
                  ModFrequencyStep = (ReferenceFrequencyToMatch / RtoMatch / ModToMatch)
                  FracToMatch = (FrequencyRemainder / ModFrequencyStep)
                  if ((FracToMatch - int(FracToMatch)) >= 0.5):
                    FracToMatch = math.ceil(FracToMatch)
                  else:
                    FracToMatch = math.floor(FracToMatch)
                  if FracToMatch >= ModToMatch:
                    FracToMatch = (ModToMatch - 1)
                  NewFrequencyError = (FrequencyRemainder - (ModFrequencyStep * FracToMatch))
                  if NewFrequencyError < 0: # convert to a positive if necessary
                    NewFrequencyError *= (-1)
                    NegativeFrequencyError = True
                  else:
                    NegativeFrequencyError = False
                  if NewFrequencyError < FrequencyError:
                    FinalReferenceFrequency = ReferenceFrequencyToMatch
                    FrequencyError = NewFrequencyError
                    MatchingR = RtoMatch
                    MatchingInt = IntTemp
                    MatchingMod = ModToMatch
                    MatchingFrac = FracToMatch
                  if FrequencyError == 0: # exact frequency can be obtained
                    break
              if FrequencyError == 0: # exact frequency can be obtained
                break
      if FrequencyError == 0:
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
    print("Reference frequency (Hz):", FinalReferenceFrequency)
  else:
    print("R and/or Int values within datasheet limits are not possible with specified reference and RF frequencies")