// EnvelopeFollower.h
#pragma once

#include <algorithm>
#include <cmath>

class EnvelopeFollower {
public:
  enum Mode {
    PEAK = 0, // Fast attack, slow release (punchy)
    RMS,      // Average power (smooth)
    VINTAGE,  // Asymmetric like analog (musical)
    VACTROL   // Vactrol-style opto behavior
  };

  EnvelopeFollower() { Reset(); }

  // ==========================================
  // Setup Functions
  // ==========================================
  void Initialize(float sampleRate) {
    mSampleRate = sampleRate;
    UpdateCoefficients();
    Reset();
  }

  void Reset() {
    mEnvelope = 0.0f;
    mRmsState = 0.0f;
    mPeakHold = 0.0f;
    mFollowerState = 0.0f;
    mVactrolState = 0.0f;
    mVactrolMemory = 0.0f;
  }

  // ==========================================
  // Parameter Controls
  // ==========================================

  // Attack time in milliseconds (how fast it responds to increases)
  void SetAttack(float attackMs) {
    mAttackMs = std::max(0.01f, std::min(attackMs, 1000.0f));
    UpdateCoefficients();
  }

  // Release time in milliseconds (how fast it falls back)
  void SetRelease(float releaseMs) {
    mReleaseMs = std::max(1.0f, std::min(releaseMs, 5000.0f));
    UpdateCoefficients();
  }

  // Sensitivity/Threshold (-60 to 0 dB)
  void SetSensitivity(float sensitivityDb) {
    mSensitivity = std::pow(10.0f, sensitivityDb / 20.0f);
  }

  // Amount of modulation (0 to 1)
  void SetAmount(float amount) {
    mAmount = std::max(0.0f, std::min(amount, 1.0f));
  }

  // Set follower mode
  void SetMode(Mode mode) { mMode = mode; }

  // Smoothing for the output (reduces jitter)
  void SetSmoothing(float smoothingMs) {
    mSmoothingMs = std::max(0.1f, std::min(smoothingMs, 100.0f));
    UpdateCoefficients();
  }

  // Set curve shape (0.0 to 1.0)
  void SetCurve(float curve) { mCurve = std::max(0.0f, std::min(curve, 1.0f)); }

  // ==========================================
  // Main Processing
  // ==========================================

  // Process a single sample and return envelope value (0 to 1)
  float ProcessSample(float input) {
    // Get absolute value for envelope
    float rectified = std::abs(input);

    // Apply sensitivity scaling
    rectified *= mSensitivity;

    // Process based on mode
    float targetEnvelope = 0.0f;

    switch (mMode) {
    case PEAK:
      targetEnvelope = ProcessPeakMode(rectified);
      break;

    case RMS:
      targetEnvelope = ProcessRmsMode(rectified);
      break;

    case VINTAGE:
      targetEnvelope = ProcessVintageMode(rectified);
      break;

    case VACTROL:
      targetEnvelope = ProcessVactrolMode(rectified);
      break;
    }

    // Apply attack/release ballistics (except VACTROL which has its own)
    if (mMode != VACTROL) {
      float rate = (targetEnvelope > mEnvelope) ? mAttackCoeff : mReleaseCoeff;
      mEnvelope = targetEnvelope + (mEnvelope - targetEnvelope) * rate;
    } else {
      mEnvelope = targetEnvelope;
    }

    // Apply output smoothing
    mFollowerState = mEnvelope + (mFollowerState - mEnvelope) * mSmoothCoeff;

    // Apply curve shaping - much gentler to avoid artifacts
    float shapedOutput = mFollowerState;

    if (mCurve > 0.01f) {
      // Scale down the curve - maximum exponent of 1.2 instead of 1.5
      float expFactor = 1.0f + (mCurve * 0.2f); // Range 1.0 to 1.2
      shapedOutput = std::pow(mFollowerState, expFactor);
    }

    // Apply amount scaling and clamp
    float output = shapedOutput * mAmount;
    return std::max(0.0f, std::min(output, 1.0f));
  }

  // Process stereo (returns max of both channels)
  float ProcessStereo(float left, float right) {
    // Don't double-process - just take the max and process once
    float maxEnv = std::max(std::abs(left), std::abs(right));
    return ProcessSample(maxEnv);
  }

  // Get current envelope value without processing (for meters)
  float GetEnvelope() const { return mFollowerState; }

  // Get envelope in dB (for display)
  float GetEnvelopeDb() const {
    if (mFollowerState < 0.000001f)
      return -120.0f;
    return 20.0f * std::log10(mFollowerState);
  }

private:
  // ==========================================
  // Internal Processing
  // ==========================================

  float ProcessPeakMode(float input) {
    // Simple peak detection - just return the input
    // The attack/release is handled in the main process
    return input;
  }

  float ProcessRmsMode(float input) {
    // RMS averaging for smoother response
    float squared = input * input;
    mRmsState = squared + (mRmsState - squared) * mRmsCoeff;
    return std::sqrt(mRmsState);
  }

  float ProcessVintageMode(float input) {
    // Asymmetric response like analog
    // Faster on transients, musical release
    if (input > mPeakHold) {
      mPeakHold = input; // Instant attack on peaks
    } else {
      mPeakHold *= mVintageRelease; // Smooth decay
    }
    return mPeakHold;
  }

  float ProcessVactrolMode(float input) {
    // Vactrol-style opto behavior
    // Fast attack with slight slew, slow logarithmic release

    if (input > mVactrolState) {
      // Attack: Fast but with natural slew (LED turn-on)
      // Make it snappier for transients
      float attackSpeed = mVactrolAttack;

      // If it's a big transient, attack even faster
      float transientSize = input - mVactrolState;
      if (transientSize > 0.1f) {
        attackSpeed *= 0.5f; // Twice as fast for big transients
      }

      mVactrolState = input - (input - mVactrolState) * attackSpeed;
    } else {
      // Release: Slow with memory effect (photoresistor decay)
      // Add the characteristic vactrol "hang time"
      mVactrolState = mVactrolState * mVactrolRelease;

      // Memory effect - the photoresistor doesn't instantly go dark
      mVactrolMemory = mVactrolMemory * 0.95f + mVactrolState * 0.05f;

      // Blend in some memory for that vactrol "sag"
      mVactrolState = mVactrolState * 0.9f + mVactrolMemory * 0.1f;
    }

    return mVactrolState;
  }

  void UpdateCoefficients() {
    if (mSampleRate <= 0)
      return;

    // Attack/Release coefficients
    mAttackCoeff = std::exp(-1.0f / (mAttackMs * 0.001f * mSampleRate));
    mReleaseCoeff = std::exp(-1.0f / (mReleaseMs * 0.001f * mSampleRate));

    // RMS averaging coefficient (10ms window)
    mRmsCoeff = std::exp(-1.0f / (0.010f * mSampleRate));

    // Vintage mode release
    mVintageRelease = std::exp(-1.0f / (mReleaseMs * 0.0005f * mSampleRate));

    // Vactrol-style coefficients
    // Attack: Snappy but not instant (1/3 of the set attack time for punch)
    float vactrolAttackMs = mAttackMs * 0.3f;
    mVactrolAttack = std::exp(-1.0f / (vactrolAttackMs * 0.001f * mSampleRate));

    // Release: Slower for that vactrol hang (1.5x the set release time)
    float vactrolReleaseMs = mReleaseMs * 1.5f;
    mVactrolRelease =
        std::exp(-1.0f / (vactrolReleaseMs * 0.001f * mSampleRate));

    // Output smoothing
    mSmoothCoeff = std::exp(-1.0f / (mSmoothingMs * 0.001f * mSampleRate));
  }

private:
  // Parameters
  float mSampleRate = 44100.0f;
  float mAttackMs = 10.0f;    // Fast attack default
  float mReleaseMs = 100.0f;  // Medium release default
  float mSensitivity = 1.0f;  // 0 dB default
  float mAmount = 1.0f;       // Full range default
  float mSmoothingMs = 5.0f;  // Light smoothing
  float mCurve = 0.5f;        // Default linear curve
  float mCurrentCurve = 0.5f; // Actual curve value (smoothed)
  float mCurveSmoothing = 0.99f;
  Mode mMode = PEAK;

  // State variables
  float mEnvelope = 0.0f;
  float mRmsState = 0.0f;
  float mPeakHold = 0.0f;
  float mFollowerState = 0.0f;

  // Vactrol-specific state
  float mVactrolState = 0.0f;
  float mVactrolMemory = 0.0f;

  // Coefficients
  float mAttackCoeff = 0.0f;
  float mReleaseCoeff = 0.0f;
  float mRmsCoeff = 0.0f;
  float mVintageRelease = 0.0f;
  float mSmoothCoeff = 0.0f;
  float mVactrolAttack = 0.0f;
  float mVactrolRelease = 0.0f;
};
