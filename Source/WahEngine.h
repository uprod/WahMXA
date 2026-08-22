#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

namespace wahmxa
{

// Coeur DSP du wah. Trois modes de pilotage d'un meme filtre resonant (SVF
// passe-bande, topologie TPT) balaye sur 2.5 octaves :
//   UP / DOWN : un suiveur d'enveloppe mesure la force du jeu (redressement +
//   attaque/relachement) et ouvre le filtre vers l'aigu (UP) ou le referme
//   vers la base (DOWN).
//   PEDAL : la pedale bascule toute seule — un LFO en cosinus leve remplace
//   l'enveloppe ; SPEED devient la vitesse de la pedale, SENS sa course.
class WahEngine
{
public:
    static constexpr int   kMaxCh   = 2;
    static constexpr float kOctaves = 2.5f;   // etendue du balayage

    WahEngine();

    void prepare (double sampleRate, int blockSize, int numChannels);
    void reset();

    void setSensitivity (float amount01);
    void setSpeed (float amount01);
    void setBaseFreqHz (float hz);
    void setResonance (float amount01);
    void setMode (int m);              // 0 = up, 1 = down, 2 = pedale
    void setMix (float amount01);

    // Traite le buffer en place (dry/wet compris).
    void process (juce::AudioBuffer<float>& buffer);

    // --- Verites partagees avec l'UI (FIG. 1 / FIG. 2) ----------------------
    static float attackMsFor (float speed01) noexcept
    {
        return juce::jmap (juce::jlimit (0.0f, 1.0f, speed01), 25.0f, 3.0f);
    }

    static float releaseMsFor (float speed01) noexcept
    {
        return juce::jmap (juce::jlimit (0.0f, 1.0f, speed01), 400.0f, 60.0f);
    }

    static float qFor (float res01) noexcept
    {
        return 1.5f + 8.5f * juce::jlimit (0.0f, 1.0f, res01);
    }

    // Rehausse du pic : le wah gagne du corps quand la resonance monte.
    static float peakGainFor (float res01) noexcept
    {
        return std::sqrt (qFor (res01));
    }

    // Gain applique au signal redresse avant ecretage : la sensibilite.
    static float sensGainFor (float sens01) noexcept
    {
        return juce::Decibels::decibelsToGain (36.0f * juce::jlimit (0.0f, 1.0f, sens01));
    }

    // Vitesse de la pedale en mode PEDAL : SPEED devient un potard de vitesse.
    static float pedalRateHzFor (float speed01) noexcept
    {
        return 0.1f * std::pow (80.0f, juce::jlimit (0.0f, 1.0f, speed01));   // 0.1 -> 8 Hz
    }

    // Course de la pedale pour une phase donnee (0..1) : bascule en cosinus
    // leve, talon a 0, pointe a 1. Partagee avec l'UI (point de phase FIG. 2).
    static float pedalValueFor (float phase01) noexcept
    {
        const float ph = phase01 - std::floor (phase01);
        return 0.5f - 0.5f * std::cos (ph * juce::MathConstants<float>::twoPi);
    }

    // Frequence centrale pour une position de balayage donnee (0..1).
    static float fcFor (float baseHz, bool modeUp, float sweep01, double fs) noexcept
    {
        const float pos = modeUp ? sweep01 : 1.0f - sweep01;
        const float fc  = baseHz * std::pow (2.0f, kOctaves * juce::jlimit (0.0f, 1.0f, pos));
        return juce::jlimit (60.0f, 0.45f * (float) fs, fc);
    }

    // Position de balayage, frequence centrale et phase de pedale REELLES,
    // publiees pour l'UI.
    float getSweep01() const noexcept      { return uiSweep.load (std::memory_order_relaxed); }
    float getFcHz() const noexcept         { return uiFcHz.load (std::memory_order_relaxed); }
    float getPedalPhase01() const noexcept { return uiPedalPhase.load (std::memory_order_relaxed); }

private:
    double sampleRate = 44100.0;
    int    numCh = 2;

    float sens    = 0.6f;
    float speed   = 0.5f;
    float baseHz  = 350.0f;
    float res     = 0.5f;
    int   mode    = 0;
    float mix     = 1.0f;

    float envState   = 0.0f;   // suiveur d'enveloppe (signal redresse lisse)
    float pedalPhase = 0.0f;   // phase du LFO de pedale, 0..1
    float sweepState = 0.0f;   // position de balayage lissee (anti-zipper)
    float mixState   = 1.0f;
    float kSlew = 0.01f;       // coefficient un-pole (~5 ms), fixe dans prepare()
    float kSlow = 0.005f;      // idem (~10 ms) pour le mix

    // Etats du SVF TPT (Zavalishin), par canal.
    float ic1eq[kMaxCh] { 0.0f, 0.0f };
    float ic2eq[kMaxCh] { 0.0f, 0.0f };

    std::atomic<float> uiSweep      { 0.0f };
    std::atomic<float> uiFcHz       { 350.0f };
    std::atomic<float> uiPedalPhase { 0.0f };
};

}
