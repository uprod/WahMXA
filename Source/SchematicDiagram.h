#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace wahmxa
{

// FIG. 2 - Le chemin du signal dessine comme dans le manuel : IN, filtre VCF
// dont la frequence imprimee est la VRAIE frequence balayee, chaine laterale
// du suiveur d'enveloppe (attaque/relachement reels imprimes) dont le fil de
// commande a l'epaisseur de l'enveloppe VIVANTE, rails dry/wet ponderes par le
// mix. La quantite est dessinee en geometrie : le schema est la valeur.
class SchematicDiagram : public juce::Component
{
public:
    explicit SchematicDiagram (WahProcessor&);

    void paint (juce::Graphics&) override;

private:
    WahProcessor& processor;

    std::atomic<float>* sens  = nullptr;
    std::atomic<float>* speed = nullptr;
    std::atomic<float>* freq  = nullptr;
    std::atomic<float>* res   = nullptr;
    std::atomic<float>* mode  = nullptr;
    std::atomic<float>* mix   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchematicDiagram)
};

}
