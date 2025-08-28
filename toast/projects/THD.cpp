// Source/DSP/TransformerTHD.cpp
#include "TransformerTHD.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define static constants
const float TransformerTHD::DC_BLOCKER_FREQ = 20.0f;
const float TransformerTHD::LOW_SHELF_FREQ = 100.0f;
const float TransformerTHD::HIGH_DAMPEN_FREQ = 8000.0f;

// Constructor
TransformerTHD::TransformerTHD()
    : sampleRate(44100.0f), hysteresisState(0.0f), dcBlockerState(0.0f),
      dcBlockerPrevInput(0.0f), dcBlockerPrevOutput(0.0f), lowShelfState1(0.0f),
      lowShelfState2(0.0f), highDampenState(0.0f), thdAmount(0.3f),
      warmth(0.5f), asymmetry(0.15f), hysteresisAmount(0.2f) {}

void TransformerTHD::Initialize(float newSampleRate) {
  sampleRate = newSampleRate;
  Reset();
}

void TransformerTHD::Reset() {
  hysteresisState = 0.0f;
  dcBlockerState = 0.0f;
  dcBlockerPrevInput = 0.0f;
  dcBlockerPrevOutput = 0.0f;
  lowShelfState1 = 0.0f;
  lowShelfState2 = 0.0f;
  highDampenState = 0.0f;
}

void TransformerTHD::SetTHDAmount(float amount) {
  thdAmount = std::max(0.0f, std::min(1.0f, amount));
}

void TransformerTHD::SetWarmth(float amount) {
  warmth = std::max(0.0f, std::min(1.0f, amount));
}

void TransformerTHD::SetAsymmetry(float amount) {
  asymmetry = std::max(0.0f, std::min(1.0f, amount));
}

void TransformerTHD::SetHysteresis(float amount) {
  hysteresisAmount = std::max(0.0f, std::min(1.0f, amount));
}

float TransformerTHD::ProcessSample(float inputSample) {
  // Safety check for invalid input
  if (std::isnan(inputSample) || std::isinf(inputSample)) {
    return 0.0f;
  }

  // Clamp input to prevent extreme values
  float sample = std::max(-2.0f, std::min(2.0f, inputSample));

  // Stage 1: Pre-emphasis (frequency shaping)
  sample = ApplyLowShelf(sample);

  // Stage 2: Harmonic Generation
  sample = ApplyHysteresis(sample);
  sample = ApplyAsymmetricSaturation(sample);

  // Stage 3: Post-processing
  sample = ApplyHighDampening(sample);
  sample = ApplyDCBlocker(sample);
  sample = SoftLimit(sample);

  return sample;
}

float TransformerTHD::ApplyAsymmetricSaturation(float input) {
  // More conservative drive range
  float drive = 1.0f + (thdAmount * 4.0f); // Reduced from 8.0f
  float x = input * drive;

  // Asymmetry for even harmonics
  float bias = asymmetry * 0.2f; // Reduced from 0.3f
  x += bias;

  // Waveshaping
  float x2 = x * x;
  float x3 = x2 * x;

  // Mix of different saturation curves
  float tanh_sat = x * (27.0f + x2) / (27.0f + 9.0f * x2);
  float cubic_sat = x - (x3 * 0.1f);

  // Blend the two saturation types
  float saturated = tanh_sat * 0.7f + cubic_sat * 0.3f;

  // Remove DC offset from asymmetry
  saturated -= bias * 0.5f;

  // BETTER GAIN COMPENSATION - This is key!
  float compensation =
      1.0f / (1.0f + thdAmount * 3.5f); // More aggressive compensation

  // Adjust wetness to start at 0 when THD is at 0
  float wetness = thdAmount; // Linear from 0 to 1, no minimum wetness!

  // When THD is 0, output = 100% dry (no volume change)
  return input * (1.0f - wetness) + saturated * wetness * compensation;
}
float TransformerTHD::ApplyHysteresis(float input) {
  float hyst = hysteresisAmount * 0.05f;
  float diff = input - hysteresisState;

  if (std::abs(diff) > hyst) {
    hysteresisState += diff * 0.8f;
  } else {
    hysteresisState += diff * 0.3f;
  }

  return input * (1.0f - hysteresisAmount * 0.2f) +
         hysteresisState * (hysteresisAmount * 0.2f);
}

float TransformerTHD::ApplyLowShelf(float input) {
  float freq = LOW_SHELF_FREQ / sampleRate;
  float w = 2.0f * M_PI * freq;
  float cosw = std::cos(w);
  float sinw = std::sin(w);

  float gain = 1.0f + (warmth * 0.3f);
  float S = 1.0f;
  float A = std::sqrt(gain);

  float alpha = sinw / 2.0f * std::sqrt((A + 1 / A) * (1 / S - 1) + 2);
  float a0 = (A + 1) + (A - 1) * cosw + 2 * std::sqrt(A) * alpha;

  lowShelfState1 = lowShelfState1 * 0.95f + input * 0.05f * gain;
  return input + lowShelfState1 * warmth * 0.3f;
}

float TransformerTHD::ApplyHighDampening(float input) {
  float freq = HIGH_DAMPEN_FREQ / sampleRate;
  float alpha = std::exp(-2.0f * M_PI * freq);
  float dampenAmount = thdAmount * 0.3f;

  highDampenState = input * (1.0f - alpha) + highDampenState * alpha;

  return input * (1.0f - dampenAmount) + highDampenState * dampenAmount;
}

float TransformerTHD::ApplyDCBlocker(float input) {
  float freq = DC_BLOCKER_FREQ / sampleRate;
  float alpha = 1.0f / (1.0f + 2.0f * M_PI * freq);

  float output =
      input - dcBlockerPrevInput + dcBlockerPrevOutput * (1.0f - alpha);

  dcBlockerPrevInput = input;
  dcBlockerPrevOutput = output;

  return output;
}

float TransformerTHD::SoftLimit(float input) {
  if (std::abs(input) > 0.95f) {
    float sign = (input > 0) ? 1.0f : -1.0f;
    float excess = std::abs(input) - 0.95f;
    return sign * (0.95f + std::tanh(excess * 2.0f) * 0.05f);
  }
  return input;
}
