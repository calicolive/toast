// Source/DSP/TransformerTHD.h
#pragma once

#include <algorithm>
#include <cmath>

class TransformerTHD {
private:
  // State Variables
  float sampleRate;
  float hysteresisState;
  float dcBlockerState;
  float dcBlockerPrevInput;
  float dcBlockerPrevOutput;
  float lowShelfState1;
  float lowShelfState2;
  float highDampenState;

  // User Parameters
  float thdAmount;
  float warmth;
  float asymmetry;
  float hysteresisAmount;

  // Internal Constants
  static const float DC_BLOCKER_FREQ;
  static const float LOW_SHELF_FREQ;
  static const float HIGH_DAMPEN_FREQ;

public:
  // Constructor
  TransformerTHD();

  // Public methods
  void Initialize(float newSampleRate);
  void Reset();
  void SetTHDAmount(float amount);
  void SetWarmth(float amount);
  void SetAsymmetry(float amount);
  void SetHysteresis(float amount);
  float ProcessSample(float inputSample);

private:
  // Private methods
  float ApplyAsymmetricSaturation(float input);
  float ApplyHysteresis(float input);
  float ApplyLowShelf(float input);
  float ApplyHighDampening(float input);
  float ApplyDCBlocker(float input);
  float SoftLimit(float input);
};
