// THD.h
#pragma once

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class TransformerTHD {
private:
    // ==========================================
    // State Variables
    // ==========================================
    
    // Sample rate for time-based calculations
    float sampleRate = 44100.0f;
    
    // Hysteresis state - models magnetic memory
    float hysteresisState = 0.0f;
    
    // DC blocking filter state
    float dcBlockerState = 0.0f;
    float dcBlockerPrevInput = 0.0f;
    float dcBlockerPrevOutput = 0.0f;
    
    // Low-frequency enhancement filter states
    float lowShelfState1 = 0.0f;
    float lowShelfState2 = 0.0f;
    
    // High-frequency dampening states
    float highDampenState = 0.0f;
    
    // ==========================================
    // User Parameters (0.0 to 1.0 range)
    // ==========================================
    
    float thdAmount = 0.3f;        // Amount of harmonic coloration (0-1)
    float warmth = 0.5f;            // Low frequency emphasis (0-1)
    float asymmetry = 0.15f;        // Even harmonic generation (0-1)
    float hysteresisAmount = 0.2f;  // Magnetic-style memory effect (0-1)
    
    // ==========================================
    // Internal Constants
    // ==========================================
    
    const float DC_BLOCKER_FREQ = 20.0f;  // Hz - removes DC offset
    const float LOW_SHELF_FREQ = 100.0f;  // Hz - warmth frequency
    const float HIGH_DAMPEN_FREQ = 8000.0f; // Hz - smooth highs
    
public:
    // Constructor
    TransformerTHD();
    
    // Initialization
    void Initialize(float newSampleRate);
    void Reset();
    
    // Parameter Control
    void SetTHDAmount(float amount);
    void SetWarmth(float amount);
    void SetAsymmetry(float amount);
    void SetHysteresis(float amount);
    
    // Main Processing
    float ProcessSample(float inputSample);
    
private:
    // Internal Processing Functions
    float ApplyAsymmetricSaturation(float input);
    float ApplyHysteresis(float input);
    float ApplyLowShelf(float input);
    float ApplyHighDampening(float input);
    float ApplyDCBlocker(float input);
    float SoftLimit(float input);
};
