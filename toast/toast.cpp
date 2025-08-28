#include "toast.h"
#include "IPlug_include_in_plug_src.h"
#include "IPlugPaths.h"

toast::toast(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
    GetParam(kParamDrive)->InitDouble("Input", 0.0, -12.0, 12.0, 0.1, "dB");
    GetParam(kParamTHDAmount)->InitDouble("Drive", 30.0, 0.0, 100.0, 0.1, "%");
    GetParam(kParamDynamics)->InitDouble("Dynamics", 0.0, -100.0, 100.0, 1.0, "%");
    GetParam(kParamThreshold)->InitDouble("Threshold", -20.0, -60.0, 0.0, 0.5, "dB");
    GetParam(kParamAttack)->InitDouble("Attack", 1.0, 0.5, 50.0, 0.5, "ms");
    GetParam(kParamRelease)->InitDouble("Release", 120.0, 10.0, 500.0, 1.0, "ms");
    GetParam(kParamCurve)->InitDouble("Curve", 50.0, 0.0, 100.0, 1.0, "%");
    GetParam(kParamMix)->InitDouble("Mix", 100.0, 0.0, 100.0, 0.1, "%");
    GetParam(kParamOutput)->InitDouble("Output", 0.0, -12.0, 12.0, 0.1, "dB");
    GetParam(kParamLinkGain)->InitBool("Link", true);
#ifdef DEBUG
  SetCustomUrlScheme("iplug2");
  SetEnableDevTools(true);
#endif
  
  mEditorInitFunc = [&]() {
     LoadIndexHtml(__FILE__, GetBundleID());
      LoadURL("http://localhost:5173/");
    EnableScroll(false);
  };
}

void toast::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
    const int nChans = NOutChansConnected();
    
    // Set THD parameters
    leftTHD.SetWarmth(mWarmth);
    leftTHD.SetAsymmetry(mAsymmetry);
    leftTHD.SetHysteresis(mHysteresis);
    
    if (nChans >= 2) {
        rightTHD.SetWarmth(mWarmth);
        rightTHD.SetAsymmetry(mAsymmetry);
        rightTHD.SetHysteresis(mHysteresis);
    }
    
    // Buffers for processing
    double processedBuffer[2][4096];
    double dryBuffer[2][4096];
    double* processedPtrs[2] = { processedBuffer[0], processedBuffer[1] };
    
    // Check if we should bypass (host bypass state)
    bool shouldBypass = !mHostIsActive;
    
    // Detect bypass state changes
    if (shouldBypass != mBypassState) {
        mBypassState = shouldBypass;
        mBypassFading = true;
        mBypassFadeCounter = 0;
    }
    
    // Process each sample
    for (int s = 0; s < nFrames; s++) {
        // Get smoothed parameters
        double targetDriveDB = GetParam(kParamDrive)->Value();
        double targetOutputDB = GetParam(kParamOutput)->Value();
        double targetTHDAmount = GetParam(kParamTHDAmount)->Value() / 100.0;
        double targetDynamics = GetParam(kParamDynamics)->Value() / 100.0;
        double targetMix = GetParam(kParamMix)->Value() / 100.0;
        
        double smoothedDriveDB = mDriveSmooth.Process(targetDriveDB);
        double smoothedOutputDB = mOutputSmooth.Process(targetOutputDB);
        double smoothedTHDAmount = mTHDAmountSmooth.Process(targetTHDAmount);
        double smoothedDynamics = mDynamicsSmooth.Process(targetDynamics);
        double smoothedMix = mMixSmooth.Process(targetMix);
        
        double smoothedDriveGain = std::pow(10.0, smoothedDriveDB / 20.0);
        double smoothedOutputGain = std::pow(10.0, smoothedOutputDB / 20.0);
        double smoothedDryGain = 1.0 - smoothedMix;
        double smoothedWetGain = smoothedMix;
        
        // Get envelope
        float rawEnvelope = 0.0f;
        if (nChans >= 2) {
            rawEnvelope = mEnvelopeFollower.ProcessStereo(inputs[0][s], inputs[1][s]);
        } else if (nChans >= 1) {
            rawEnvelope = mEnvelopeFollower.ProcessSample(inputs[0][s]);
        }
        
        // Calculate modulation
        float envelopeDb = -120.0f;
        if (rawEnvelope > 0.000001f) {
            envelopeDb = 20.0f * std::log10(rawEnvelope);
        }
        
        float thresholdedEnvelope = 0.0f;
        if (envelopeDb > mThresholdDb) {
            float headroom = 0.0f - (float)mThresholdDb;
            float aboveThreshold = envelopeDb - (float)mThresholdDb;
            thresholdedEnvelope = std::min(1.0f, aboveThreshold / headroom);
        }
        
        float modulatedThd = (float)smoothedTHDAmount;
        float modulation = thresholdedEnvelope * (float)smoothedDynamics;
        modulatedThd = std::max(0.0f, std::min(modulatedThd + modulation, 1.0f));
        
        leftTHD.SetTHDAmount(modulatedThd);
        if (nChans >= 2) {
            rightTHD.SetTHDAmount(modulatedThd);
        }
        
        // Process left channel
        if (nChans >= 1) {
            double input = inputs[0][s];
            dryBuffer[0][s] = input;
            
            // Always process to keep THD warm
            double driven = input * smoothedDriveGain;
            double processed = leftTHD.ProcessSample(driven);
            double wet = processed * smoothedOutputGain;
            double mixed = (input * smoothedDryGain) + (wet * smoothedWetGain);
            processedBuffer[0][s] = mixed;
        }
        
        // Process right channel
        if (nChans >= 2) {
            double input = inputs[1][s];
            dryBuffer[1][s] = input;
            
            double driven = input * smoothedDriveGain;
            double processed = rightTHD.ProcessSample(driven);
            double wet = processed * smoothedOutputGain;
            double mixed = (input * smoothedDryGain) + (wet * smoothedWetGain);
            processedBuffer[1][s] = mixed;
        }
        
        mEnvelopeValue = thresholdedEnvelope;
        mModulatedTHDAmount = modulatedThd;
    }
    
    // Apply DC blocking to processed signal
    mDCBlocker.ProcessBlock(processedPtrs, processedPtrs, nChans, nFrames);
    
    // Output with bypass crossfade
    for (int c = 0; c < nChans; c++) {
        for (int s = 0; s < nFrames; s++) {
            if (mBypassFading && mBypassFadeCounter < kBypassFadeSamples) {
                double fadeProgress = (double)mBypassFadeCounter / (double)kBypassFadeSamples;
                double fadeCurve = 0.5 * (1.0 - std::cos(M_PI * fadeProgress));
                
                if (mBypassState) {
                    // Fading to bypass
                    outputs[c][s] = processedBuffer[c][s] * (1.0 - fadeCurve) + dryBuffer[c][s] * fadeCurve;
                } else {
                    // Fading to active
                    outputs[c][s] = dryBuffer[c][s] * (1.0 - fadeCurve) + processedBuffer[c][s] * fadeCurve;
                }
                
                if (c == 0) mBypassFadeCounter++;
                if (mBypassFadeCounter >= kBypassFadeSamples) mBypassFading = false;
            } else {
                outputs[c][s] = mBypassState ? dryBuffer[c][s] : processedBuffer[c][s];
            }
        }
    }
    
    // Pass through      additional channels
    for (int c = nChans; c < NOutChansConnected(); c++) {
        for (int s = 0; s < nFrames; s++) {
            outputs[c][s] = (c < NInChansConnected()) ? inputs[c][s] : 0.0;
        }
    }
    mSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
}

void toast::OnReset()
{
    // Initialize THD processors
    leftTHD.Initialize(GetSampleRate());
    rightTHD.Initialize(GetSampleRate());
    
    // Warm up processors
    for (int i = 0; i < 512; i++) {
        leftTHD.ProcessSample(0.0);
        rightTHD.ProcessSample(0.0);
    }
    
    // Configure envelope follower
    mEnvelopeFollower.Initialize(GetSampleRate());
    mEnvelopeFollower.SetMode(EnvelopeFollower::RMS);
    mEnvelopeFollower.SetSensitivity(1.0f);
    mEnvelopeFollower.SetAttack(mAttackMs);
    mEnvelopeFollower.SetRelease(mReleaseMs);
    mEnvelopeFollower.SetSmoothing(1.0f);
    mEnvelopeFollower.SetCurve(mCurveValue);
    mEnvelopeFollower.SetAmount(1.0f);
    mEnvelopeFollower.Reset();
    
    // Initialize smoothers
    const double gainSmoothingMs = 50.0;
    const double paramSmoothingMs = 30.0;
    
    double initialDriveDB = GetParam(kParamDrive)->Value();
    double initialOutputDB = GetParam(kParamOutput)->Value();
    double initialTHDAmount = GetParam(kParamTHDAmount)->Value() / 100.0;
    double initialDynamics = GetParam(kParamDynamics)->Value() / 100.0;
    double initialMix = GetParam(kParamMix)->Value() / 100.0;
    
    mDriveSmooth = LogParamSmooth<double>(gainSmoothingMs, initialDriveDB);
    mDriveSmooth.SetSmoothTime(gainSmoothingMs, GetSampleRate());
    mDriveSmooth.SetValue(initialDriveDB);
    
    mOutputSmooth = LogParamSmooth<double>(gainSmoothingMs, initialOutputDB);
    mOutputSmooth.SetSmoothTime(gainSmoothingMs, GetSampleRate());
    mOutputSmooth.SetValue(initialOutputDB);
    
    mTHDAmountSmooth = LogParamSmooth<double>(paramSmoothingMs, initialTHDAmount);
    mTHDAmountSmooth.SetSmoothTime(paramSmoothingMs, GetSampleRate());
    mTHDAmountSmooth.SetValue(initialTHDAmount);
    
    mDynamicsSmooth = LogParamSmooth<double>(paramSmoothingMs, initialDynamics);
    mDynamicsSmooth.SetSmoothTime(paramSmoothingMs, GetSampleRate());
    mDynamicsSmooth.SetValue(initialDynamics);
    
    mMixSmooth = LogParamSmooth<double>(paramSmoothingMs, initialMix);
    mMixSmooth.SetSmoothTime(paramSmoothingMs, GetSampleRate());
    mMixSmooth.SetValue(initialMix);
    
    // Reset state
    mEnvelopeValue = 0.0f;
    mModulatedTHDAmount = (float)initialTHDAmount;
    mDryGain = 1.0 - initialMix;
    mWetGain = initialMix;
    
    mBypassState = false;
    mBypassFading = false;
    mBypassFadeCounter = 0;
    mHostIsActive = true;
}

void toast::OnActivate(bool active){
    mHostIsActive = active;
}

void toast::OnParamChange(int paramIdx){
    switch (paramIdx) {
        case kParamDrive:
        {
            double driveDB = GetParam(kParamDrive)->Value();
            
            if (mLinkGain && !mUpdatingLinkedParam) {
                mUpdatingLinkedParam = true;
                double compensationDB = -driveDB;
                GetParam(kParamOutput)->Set(compensationDB);
                mUpdatingLinkedParam = false;
            }
        }
        break;
        
        case kParamThreshold:
            mThresholdDb = GetParam(kParamThreshold)->Value();
            break;
        
        case kParamAttack:
            mAttackMs = GetParam(kParamAttack)->Value();
            mEnvelopeFollower.SetAttack(mAttackMs);
            break;
        
        case kParamRelease:
            mReleaseMs = GetParam(kParamRelease)->Value();
            mEnvelopeFollower.SetRelease(mReleaseMs);
            break;
        
        case kParamCurve:
            mCurveValue = GetParam(kParamCurve)->Value() / 100.0;
            mEnvelopeFollower.SetCurve(mCurveValue);
            break;
        
        case kParamOutput:
        {
            double outputDB = GetParam(kParamOutput)->Value();
            
            if (mLinkGain && !mUpdatingLinkedParam) {
                mUpdatingLinkedParam = true;
                double compensationDB = -outputDB;
                GetParam(kParamDrive)->Set(compensationDB);
                mUpdatingLinkedParam = false;
            } else if (!mLinkGain) {
                mUserOutputDB = outputDB;
            }
        }
        break;
        
        case kParamLinkGain:
        {
            bool wasLinked = mLinkGain;
            mLinkGain = GetParam(kParamLinkGain)->Bool();
            
            if (mLinkGain && !wasLinked) {
                double driveDB = GetParam(kParamDrive)->Value();
                double compensationDB = -driveDB;
                
                mUpdatingLinkedParam = true;
                GetParam(kParamOutput)->Set(compensationDB);
                mUpdatingLinkedParam = false;
            } else if (!mLinkGain && wasLinked) {
                GetParam(kParamOutput)->Set(mUserOutputDB);
            }
        }
        break;
        
        default:
            break;
    }
}

void toast::OnIdle()
{
  mSender.TransmitData(*this);
}
