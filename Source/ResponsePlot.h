#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace wahmxa
{

// FIG. 1 - Reponse en frequence du filtre A LA POSITION REELLE du balayage,
// tracee comme la figure d'un manuel technique. Ce n'est pas une illustration :
// la courbe est la fonction de transfert du prototype analogique du SVF
// passe-bande (rehausse de pic et mix compris), evaluee a la frequence centrale
// que le suiveur d'enveloppe impose au moteur en ce moment meme : quand on
// joue, le pic danse. Le repaint est pilote par le Timer de l'editeur (~30 Hz).
class ResponsePlot : public juce::Component
{
public:
    explicit ResponsePlot (WahProcessor&);

    void paint (juce::Graphics&) override;

private:
    WahProcessor& processor;

    std::atomic<float>* freq = nullptr;
    std::atomic<float>* res  = nullptr;
    std::atomic<float>* mode = nullptr;
    std::atomic<float>* mix  = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResponsePlot)
};

}
