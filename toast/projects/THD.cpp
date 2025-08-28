// THD.cpp
#include "THD.h"

// Constructor
TransformerTHD::TransformerTHD() {
    Reset();
}

// ==========================================
// Initialization
// ==========================================
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

// ==========================================
// Parameter Control
// ==========================================
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

// ==========================================
// Main Processing Function
// ==========================================
float TransformerTHD::ProcessSample(float inputSample) {
    if (std::isnan(inputSample) || std::isinf(inputSample)) {
        return 0.0f;
    }
    
    float sample = std::max(-2.0f, std::min(2.0f, inputSample));
    
    // Process in order for maximum interaction between effects
    sample = ApplyLowShelf(sample);      // Warmth first
    sample = ApplyHysteresis(sample);    // Then compression/lag
    sample = ApplyAsymmetricSaturation(sample);  // Then saturation
    sample = ApplyHighDampening(sample); // Smooth the harmonics
    sample = ApplyDCBlocker(sample);     // Remove DC
    sample = SoftLimit(sample);          // Final safety
    
    return sample;
}

// ==========================================
// Saturation Functions
// ==========================================

// FIXED: More audible saturation with better harmonic generation
float TransformerTHD::ApplyAsymmetricSaturation(float input) {
    float drive = 1.0f + (thdAmount * 4.0f);
    float x = input * drive;
    
    // SUBTLE ASYMMETRY - back to musical territory
    // Just enough to color the tone without distortion
    float bias = asymmetry * 0.15f;  // Subtle DC offset
    x += bias;
    
    // Standard waveshaping
    float x2 = x * x;
    float saturated = x * (27.0f + x2) / (27.0f + 9.0f * x2);
    
    // Remove the DC bias
    saturated -= bias * 0.8f;
    
    // Only add a tiny bit of even harmonics when asymmetry is high
    if (asymmetry > 0.5f) {
        // Very subtle 2nd harmonic only at high settings
        float secondHarmonic = std::tanh(x * 2.0f) * (asymmetry - 0.5f) * 0.05f;  // VERY subtle
        saturated += secondHarmonic;
    }
    
    // Gain compensation
    float compensation = 1.0f / (1.0f + thdAmount * 3.5f);
    float wetness = thdAmount;
    
    return input * (1.0f - wetness) + saturated * wetness * compensation;
}

// FIXED: More audible hysteresis effect
float TransformerTHD::ApplyHysteresis(float input) {
    if (hysteresisAmount < 0.01f) {
        hysteresisState = input;
        return input;
    }
    
    // PROBLEM: The old version was acting like a low-pass filter
    // SOLUTION: Make it add compression/thickness WITHOUT filtering
    
    // Magnetic-style saturation with memory
    float targetState = input;
    
    // Non-linear rate based on difference
    float diff = targetState - hysteresisState;
    float rate;
    
    // Make the rate frequency-dependent to preserve highs
    float absDiff = std::abs(diff);
    if (absDiff > 0.1f) {
        // Fast response for transients (preserves high freq)
        rate = 0.8f - (hysteresisAmount * 0.3f);  // Still pretty fast
    } else {
        // Slower for sustaining notes
        rate = 0.4f - (hysteresisAmount * 0.3f);
    }
    
    // Update state
    hysteresisState += diff * rate;
    
    // Add "magnetic" saturation (not filtering!)
    float magnetic = hysteresisState;
    
    // Soft saturation with memory - NO high frequency loss
    if (std::abs(magnetic) > 0.3f) {
        float sign = (magnetic > 0) ? 1.0f : -1.0f;
        float excess = std::abs(magnetic) - 0.3f;
        // Gentle compression
        magnetic = sign * (0.3f + excess / (1.0f + excess * hysteresisAmount * 2.0f));
    }
    
    // IMPORTANT: Mix with DRY signal to preserve high frequencies
    // The more hysteresis, the more "thickness" without losing highs
    float mix = hysteresisAmount * 0.5f;  // Never more than 50% wet
    
    // Add back some high-frequency content that might have been smoothed
    float highFreqCompensation = (input - hysteresisState) * hysteresisAmount * 0.3f;
    
    return input * (1.0f - mix) + magnetic * mix + highFreqCompensation;
}


// ==========================================
// Filtering Functions
// ==========================================

// FIXED: Proper low shelf that adds warmth without killing signal
float TransformerTHD::ApplyLowShelf(float input) {
    if (warmth < 0.01f) {
        return input;
    }
    
    // Optimized for always-on use at 100%
    // Should add body without muddiness
    
    // Extract bass (below 200Hz)
    float bassFreq = 200.0f / sampleRate;
    float rc1 = 1.0f - std::exp(-2.0f * M_PI * bassFreq);
    lowShelfState1 += rc1 * (input - lowShelfState1);
    
    // Extract sub-bass (below 80Hz)
    float subFreq = 80.0f / sampleRate;
    float rc2 = 1.0f - std::exp(-2.0f * M_PI * subFreq);
    lowShelfState2 += rc2 * (input - lowShelfState2);
    
    // Boost with frequency-dependent amounts
    float subBoost = lowShelfState2 * warmth * 0.25f;  // Controlled sub boost
    float bassBoost = (lowShelfState1 - lowShelfState2) * warmth * 0.15f;  // Low-mid warmth
    
    // Add gentle saturation to bass for harmonics
    float bassSaturated = std::tanh(lowShelfState1 * 2.0f) * warmth * 0.1f;
    
    // Since you use it at 100%, make sure it doesn't get muddy
    // Slight mid-scoop to maintain clarity
    float midScoop = 0.0f;
    if (warmth > 0.7f) {
        float midFreq = 500.0f / sampleRate;
        float midAlpha = std::exp(-2.0f * M_PI * midFreq);
        float midContent = input * (1.0f - midAlpha) + input * midAlpha;
        midScoop = (midContent - input) * (warmth - 0.7f) * 0.1f;  // Very subtle scoop
    }
    
    return input + subBoost + bassBoost + bassSaturated - midScoop;
}

// Make high dampening more subtle
float TransformerTHD::ApplyHighDampening(float input) {
    // Make this VERY subtle - the highs were being killed by hysteresis
    float freq = 15000.0f / sampleRate;  // Much higher frequency
    float alpha = std::exp(-2.0f * M_PI * freq);
    
    // Only apply when THD is very high
    float dampenAmount = thdAmount * 0.05f;  // Much less dampening
    
    highDampenState = input * (1.0f - alpha) + highDampenState * alpha;
    
    return input * (1.0f - dampenAmount) + highDampenState * dampenAmount;
}

// Keep the DC blocker gentle to preserve bass
float TransformerTHD::ApplyDCBlocker(float input) {
    // Even gentler to preserve more bass
    const float R = 0.999f;  // Increased from 0.995f
    
    dcBlockerState = input - dcBlockerPrevInput + R * dcBlockerState;
    dcBlockerPrevInput = input;
    
    return dcBlockerState;
}

// Final soft limiting
float TransformerTHD::SoftLimit(float input) {
    // Gentle limiting to prevent harsh clipping
    if (std::abs(input) > 0.95f) {
        float sign = (input > 0) ? 1.0f : -1.0f;
        float excess = std::abs(input) - 0.95f;
        return sign * (0.95f + std::tanh(excess * 2.0f) * 0.05f);
    }
    return input;
}
