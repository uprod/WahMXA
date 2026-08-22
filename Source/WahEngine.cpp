#include "WahEngine.h"

namespace wahmxa
{

WahEngine::WahEngine() = default;

void WahEngine::prepare (double newSampleRate, int /*blockSize*/, int numChannels)
{
    sampleRate = newSampleRate;
    numCh = juce::jlimit (1, kMaxCh, numChannels);

    kSlew = 1.0f - std::exp (-1.0f / (0.005f * (float) sampleRate));
    kSlow = 1.0f - std::exp (-1.0f / (0.010f * (float) sampleRate));

    reset();
}

void WahEngine::reset()
{
    envState   = 0.0f;
    pedalPhase = 0.0f;
    sweepState = 0.0f;
    mixState   = mix;
    for (int ch = 0; ch < kMaxCh; ++ch)
        ic1eq[ch] = ic2eq[ch] = 0.0f;
}

void WahEngine::setSensitivity (float a)  { sens   = juce::jlimit (0.0f, 1.0f, a); }
void WahEngine::setSpeed (float a)        { speed  = juce::jlimit (0.0f, 1.0f, a); }
void WahEngine::setBaseFreqHz (float hz)  { baseHz = juce::jlimit (60.0f, 2000.0f, hz); }
void WahEngine::setResonance (float a)    { res  = juce::jlimit (0.0f, 1.0f, a); }
void WahEngine::setMode (int m)           { mode = juce::jlimit (0, 2, m); }
void WahEngine::setMix (float a)          { mix  = juce::jlimit (0.0f, 1.0f, a); }

void WahEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int chs        = juce::jmin (numCh, buffer.getNumChannels());

    const float aAtk = 1.0f - std::exp (-1.0f / (attackMsFor (speed) * 0.001f * (float) sampleRate));
    const float aRel = 1.0f - std::exp (-1.0f / (releaseMsFor (speed) * 0.001f * (float) sampleRate));
    const float sensGain = sensGainFor (sens);
    const float q    = qFor (res);
    const float kInv = 1.0f / q;
    const float peak = peakGainFor (res);

    const bool  pedal    = (mode == 2);
    const float pedalInc = pedalRateHzFor (speed) / (float) sampleRate;

    for (int n = 0; n < numSamples; ++n)
    {
        float sweepTarget;

        if (pedal)
        {
            // La pedale bascule toute seule : SPEED est sa vitesse, SENS sa course.
            pedalPhase += pedalInc;
            if (pedalPhase >= 1.0f) pedalPhase -= 1.0f;
            sweepTarget = sens * pedalValueFor (pedalPhase);
        }
        else
        {
            // Suiveur d'enveloppe sur la somme mono redressee.
            float r = 0.0f;
            for (int ch = 0; ch < chs; ++ch)
                r += std::abs (buffer.getSample (ch, n));
            r /= (float) chs;

            envState += (r > envState ? aAtk : aRel) * (r - envState);
            sweepTarget = juce::jlimit (0.0f, 1.0f, envState * sensGain);
        }

        sweepState += kSlew * (sweepTarget - sweepState);
        mixState   += kSlow * (mix - mixState);

        const float fc = fcFor (baseHz, mode != 1, sweepState, sampleRate);
        const float g  = std::tan (juce::MathConstants<float>::pi * fc / (float) sampleRate);
        const float a1 = 1.0f / (1.0f + g * (g + kInv));

        for (int ch = 0; ch < chs; ++ch)
        {
            const float x  = buffer.getSample (ch, n);
            const float v3 = x - ic2eq[ch];
            const float v1 = a1 * (ic1eq[ch] + g * v3);
            const float v2 = ic2eq[ch] + g * v1;
            ic1eq[ch] = 2.0f * v1 - ic1eq[ch];
            ic2eq[ch] = 2.0f * v2 - ic2eq[ch];

            // Passe-bande normalise (bp x 1/Q, pic unite) + rehausse de pic.
            const float wet = v1 * kInv * peak;
            buffer.setSample (ch, n, x * (1.0f - mixState) + wet * mixState);
        }
    }

    uiSweep.store (sweepState, std::memory_order_relaxed);                       // pour FIG. 1 / FIG. 2
    uiFcHz.store (fcFor (baseHz, mode != 1, sweepState, sampleRate),
                  std::memory_order_relaxed);
    uiPedalPhase.store (pedalPhase, std::memory_order_relaxed);
}

}
