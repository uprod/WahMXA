#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace wahmxa
{

namespace IDs
{
    constexpr auto sens  = "sens";
    constexpr auto speed = "speed";
    constexpr auto freq  = "freq";
    constexpr auto res   = "res";
    constexpr auto mode  = "mode";
    constexpr auto mix   = "mix";
}

juce::AudioProcessorValueTreeState::ParameterLayout WahProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Affichages "tally" a la machine a ecrire, aussi bien dans l'editeur que
    // dans les lignes d'automation de l'hote.
    const auto pctAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });

    const auto hzAttr = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " Hz"; })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });

    // Sensibilite : gain (jusqu'a +36 dB) applique au signal redresse.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::sens, 1 },
        "Sensitivity", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.6f, pctAttr));

    // Vitesse de l'enveloppe : attaque 25->3 ms, relachement 400->60 ms.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::speed, 1 },
        "Speed", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, pctAttr));

    // Base du balayage. Skew < 1 = plus de finesse dans le grave.
    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::freq, 1 },
        "Frequency", juce::NormalisableRange<float> (150.0f, 800.0f, 1.0f, 0.5f), 350.0f, hzAttr));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::res, 1 },
        "Resonance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, pctAttr));

    // Mode UP : jouer fort ouvre vers l'aigu ; DOWN : jouer fort referme ;
    // PEDAL : la pedale bascule toute seule (SPEED = vitesse, SENS = course).
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { IDs::mode, 1 }, "Mode",
        juce::StringArray { "Up", "Down", "Pedal" }, 0));

    params.push_back (std::make_unique<P> (juce::ParameterID { IDs::mix, 1 },
        "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f, pctAttr));

    return { params.begin(), params.end() };
}

WahProcessor::WahProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

WahProcessor::~WahProcessor() = default;

void WahProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    engine.reset();
}

void WahProcessor::releaseResources()
{
    engine.reset();
}

bool WahProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& main = layouts.getMainOutputChannelSet();
    if (main != juce::AudioChannelSet::mono() && main != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == main;
}

void WahProcessor::pushParameterUpdatesToEngine()
{
    engine.setSensitivity (apvts.getRawParameterValue (IDs::sens)->load());
    engine.setSpeed       (apvts.getRawParameterValue (IDs::speed)->load());
    engine.setBaseFreqHz  (apvts.getRawParameterValue (IDs::freq)->load());
    engine.setResonance   (apvts.getRawParameterValue (IDs::res)->load());
    engine.setMode        (juce::roundToInt (apvts.getRawParameterValue (IDs::mode)->load()));
    engine.setMix         (apvts.getRawParameterValue (IDs::mix)->load());
}

void WahProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    pushParameterUpdatesToEngine();

    // Le wah agit en place, dry/wet compris : pas de copie de travail.
    engine.process (buffer);
}

juce::AudioProcessorEditor* WahProcessor::createEditor()
{
    return new WahEditor (*this);
}

void WahProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void WahProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

}

// Point d'entree du plugin JUCE — doit etre au niveau global.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wahmxa::WahProcessor();
}
