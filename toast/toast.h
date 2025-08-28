#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Smoothers.h"
#include "DCBlocker.h"
#include "ISender.h"

#include "THD.h"
#include "EnvelopeFollower.h"

using namespace iplug;

const int kNumPresets = 1;

enum EParams
{
    kParamDrive = 0,
    kParamTHDAmount,
    kParamDynamics,
    kParamThreshold,
    kParamAttack,
    kParamRelease,
    kParamCurve,
    kParamMix,
    kParamOutput,
    kParamLinkGain,
    kNumParams
};

enum EControlTags
{
  kCtrlTagMeter = 0,
};

class toast final : public Plugin
{
public:
  toast(const InstanceInfo& info);
    void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
    void OnReset() override;
    void OnParamChange(int paramIdx) override;
    void OnActivate(bool active) override;
    
    void OnIdle() override;

private:
  iplug::IPeakSender<2> mSender;
    TransformerTHD leftTHD;
    TransformerTHD rightTHD;
    
    EnvelopeFollower mEnvelopeFollower;
    
    // Parameter smoothing using LogParamSmooth
    LogParamSmooth<double> mDriveSmooth;
    LogParamSmooth<double> mOutputSmooth;
    LogParamSmooth<double> mTHDAmountSmooth;
    LogParamSmooth<double> mDynamicsSmooth;
    LogParamSmooth<double> mMixSmooth;
    
    // Parameter modulation system
    float mEnvelopeValue = 0.0f;
    float mModulatedTHDAmount = 0.0f;
    
    // User parameters
    double mThresholdDb = -20.0;
    double mAttackMs = 1.0;
    double mReleaseMs = 120.0;
    double mCurveValue = 0.5;
    bool mLinkGain = true;
    double mUserOutputDB = 0.0;
    
    // Hardcoded THD settings
    const double mWarmth = 1.0;
    const double mAsymmetry = 0.75;
    const double mHysteresis = 0.75;
    
    // Dry/wet gains
    double mDryGain = 0.0;
    double mWetGain = 1.0;
    
    // Link recursion prevention
    bool mUpdatingLinkedParam = false;
    
    // Bypass handling
    bool mHostIsActive = true;
    bool mBypassState = false;
    bool mBypassFading = false;
    int mBypassFadeCounter = 0;
    static constexpr int kBypassFadeSamples = 256;
    
    // DC blocking filter
    DCBlocker<double, 2> mDCBlocker;
    
};
