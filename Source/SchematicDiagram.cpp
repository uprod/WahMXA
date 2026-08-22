#include "SchematicDiagram.h"
#include "ManualStyle.h"
#include "WahEngine.h"

namespace wahmxa
{

namespace
{
    // Epaisseur de trait proportionnelle a une quantite 0..1 : la geometrie
    // porte la valeur, jamais la couleur seule.
    float weightFor (float amount01)
    {
        return 0.7f + 2.4f * juce::jlimit (0.0f, 1.0f, amount01);
    }

    void drawArrowHead (juce::Graphics& g, juce::Point<float> tip, juce::Point<float> dir, float size)
    {
        dir = dir / (dir.getDistanceFromOrigin() + 1.0e-6f);
        const juce::Point<float> n (-dir.y, dir.x);
        juce::Path p;
        p.addTriangle (tip, tip - dir * size + n * (size * 0.55f),
                             tip - dir * size - n * (size * 0.55f));
        g.fillPath (p);
    }

    void drawDashedLine (juce::Graphics& g, juce::Line<float> line, float thickness)
    {
        const float dashes[] = { 3.0f, 3.0f };
        g.drawDashedLine (line, dashes, 2, thickness);
    }

    // Etiquette imprimee qui interrompt le trait qu'elle chevauche : on pose
    // un cartouche couleur film derriere le texte, comme sur un vrai plan.
    void drawLabelOverLine (juce::Graphics& g, const juce::String& text,
                            juce::Rectangle<float> area, juce::Justification just)
    {
        const float tw = juce::GlyphArrangement::getStringWidth (fonts::lettering (9.0f), text);
        auto knockout = area.withSizeKeepingCentre (tw + 10.0f, area.getHeight());
        g.setColour (palette::film);
        g.fillRect (knockout);
        g.setFont (fonts::lettering (9.0f));
        g.setColour (palette::inkMid);
        g.drawText (text, area, just);
    }

    // Croix de sommateur dans un cercle (jonction "+" du schema).
    void drawSummingNode (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setColour (palette::film);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (palette::ink);
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.1f);
        g.drawLine (c.x - r * 0.5f, c.y, c.x + r * 0.5f, c.y, 1.0f);
        g.drawLine (c.x, c.y - r * 0.5f, c.x, c.y + r * 0.5f, 1.0f);
    }

    // Rail pondere : epaisseur = quantite ; a zero il degenere en tirete fin.
    void drawWeightedLine (juce::Graphics& g, juce::Line<float> line, float amount01)
    {
        if (amount01 < 0.005f)
            drawDashedLine (g, line, 0.7f);
        else
            g.drawLine (line, weightFor (amount01));
    }
}

SchematicDiagram::SchematicDiagram (WahProcessor& proc)
    : processor (proc)
{
    auto& apvts = processor.getAPVTS();
    sens  = apvts.getRawParameterValue ("sens");
    speed = apvts.getRawParameterValue ("speed");
    freq  = apvts.getRawParameterValue ("freq");
    res   = apvts.getRawParameterValue ("res");
    mode  = apvts.getRawParameterValue ("mode");
    mix   = apvts.getRawParameterValue ("mix");

    setInterceptsMouseClicks (false, false);
}

void SchematicDiagram::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    auto caption  = getLocalBounds().toFloat().removeFromBottom (16.0f);

    const float sensV  = sens->load();
    const float speedV = speed->load();
    const float freqV  = freq->load();
    const float resV   = res->load();
    const int   modeV  = juce::roundToInt (mode->load());
    const bool  pedal  = modeV == 2;
    const float mixV   = mix->load();
    juce::ignoreUnused (sensV);

    const float sweep = processor.getSweep01();
    const float fc    = processor.getFcHz();

    // Rangs horizontaux du schema : dry au-dessus, chaine laterale en dessous.
    const float dryY  = 12.0f;
    const float railY = 38.0f;
    const float scY   = 63.0f;   // ligne de la chaine laterale

    // Colonnes.
    const float inX     = 12.0f;
    const float branchX = 36.0f;
    const float envX0   = 120.0f, envX1 = 204.0f;   // bloc suiveur d'enveloppe
    const float vcfX0   = 250.0f, vcfX1 = 340.0f;   // bloc filtre
    const float ctlX    = (vcfX0 + vcfX1) * 0.5f;   // montee du fil de commande
    const float mixX    = w * 0.86f;
    const float outX    = w - 16.0f;
    const float blockH  = 28.0f;

    // --- Rail d'entree et derivation dry ------------------------------------
    g.setColour (palette::ink);
    g.drawEllipse (inX - 3.0f, railY - 3.0f, 6.0f, 6.0f, 1.1f);              // borne IN
    g.drawLine (inX + 3.0f, railY, vcfX0, railY, 1.2f);
    drawArrowHead (g, { vcfX0, railY }, { 1.0f, 0.0f }, 6.0f);
    g.fillEllipse (branchX - 2.2f, railY - 2.2f, 4.4f, 4.4f);                // noeud de derivation

    g.setColour (palette::ink.withAlpha (0.9f));
    drawWeightedLine (g, { { branchX, railY }, { branchX, dryY } }, (1.0f - mixV) * 0.75f);
    drawWeightedLine (g, { { branchX, dryY }, { mixX, dryY } }, 1.0f - mixV);
    drawWeightedLine (g, { { mixX, dryY }, { mixX, railY - 9.0f } }, 1.0f - mixV);

    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("IN", juce::Rectangle<float> (24.0f, 10.0f).withPosition (inX - 8.0f, railY - 17.0f),
                juce::Justification::centredLeft);

    drawLabelOverLine (g, "DRY PATH",
                       juce::Rectangle<float> (80.0f, 10.0f).withCentre ({ (vcfX0 + vcfX1) * 0.5f, dryY }),
                       juce::Justification::centred);

    // --- Chaine laterale : suiveur d'enveloppe ou pedale automatique ----------
    // En mode PEDAL, le bloc n'ecoute pas l'entree : pas de derivation.
    if (! pedal)
    {
        g.setColour (palette::ink.withAlpha (0.8f));
        drawDashedLine (g, { { branchX, railY }, { branchX, scY } }, 0.9f);
        drawDashedLine (g, { { branchX, scY }, { envX0, scY } }, 0.9f);
    }

    // Bloc de commande : les valeurs imprimees sont les vraies.
    {
        const juce::Rectangle<float> block (envX0, scY - 11.0f, envX1 - envX0, 22.0f);
        g.setColour (palette::film);
        g.fillRect (block);
        g.setColour (palette::ink);
        g.drawRect (block, 1.2f);

        g.setFont (fonts::lettering (9.0f));
        g.drawText (pedal ? "PEDAL LFO" : "ENV FOLLOW",
                    block.withTrimmedBottom (8.0f), juce::Justification::centred);

        const juce::String sub = pedal
            ? juce::String (WahEngine::pedalRateHzFor (speedV), 2) + " Hz"
            : juce::String (juce::roundToInt (WahEngine::attackMsFor (speedV)))
                  + "/" + juce::String (juce::roundToInt (WahEngine::releaseMsFor (speedV))) + " ms";
        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::inkMid);
        g.drawText (sub, block.withTrimmedTop (10.0f), juce::Justification::centred);
    }

    // Fil de commande : son epaisseur EST l'enveloppe vivante.
    {
        g.setColour (palette::spot.withAlpha (0.7f));
        const float wCtl = sweep < 0.005f ? 0.7f : weightFor (sweep);
        drawDashedLine (g, { { envX1, scY }, { ctlX, scY } }, wCtl);
        drawDashedLine (g, { { ctlX, scY }, { ctlX, railY + blockH * 0.5f } }, wCtl);
        g.setColour (palette::spot);
        drawArrowHead (g, { ctlX, railY + blockH * 0.5f }, { 0.0f, -1.0f }, 5.0f);

        drawLabelOverLine (g, "SWEEP " + juce::String (juce::roundToInt (sweep * 100.0f)) + " %",
                           juce::Rectangle<float> (80.0f, 10.0f)
                               .withCentre ({ (envX1 + ctlX) * 0.5f, scY }),
                           juce::Justification::centred);
    }

    // --- Bloc VCF : la frequence imprimee est la vraie ------------------------
    {
        const juce::Rectangle<float> block (vcfX0, railY - blockH * 0.5f,
                                            vcfX1 - vcfX0, blockH);
        g.setColour (palette::film);
        g.fillRect (block);
        g.setColour (palette::ink);
        g.drawRect (block, 1.2f);

        g.setFont (fonts::lettering (10.0f));
        g.drawText ("VCF", block.withTrimmedBottom (10.0f), juce::Justification::centred);

        const juce::String fcText = fc >= 1000.0f
            ? juce::String (fc / 1000.0f, 2) + " kHz"
            : juce::String ((int) fc) + " Hz";
        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::inkMid);
        g.drawText (fcText, block.withTrimmedTop (13.0f), juce::Justification::centred);
    }

    // Sens du balayage + valeurs vraies du filtre, a droite du bloc.
    // En mode PEDAL le balayage va dans les deux sens : double fleche.
    {
        const float ax = vcfX1 + 12.0f;
        g.setColour (palette::spot);
        g.drawLine (ax, railY + 4.0f, ax, railY - 4.0f, 1.4f);
        if (pedal || modeV == 0)
            drawArrowHead (g, { ax, railY - 6.0f }, { 0.0f, -1.0f }, 4.5f);
        if (pedal || modeV == 1)
            drawArrowHead (g, { ax, railY + 6.0f }, { 0.0f, 1.0f }, 4.5f);

        const juce::String note = juce::String (pedal ? "PEDAL" : (modeV == 0 ? "UP" : "DOWN"))
            + "   Q " + juce::String (WahEngine::qFor (resV), 1)
            + "   BASE " + juce::String ((int) freqV) + " Hz";
        g.setFont (fonts::mono (8.0f));
        g.setColour (palette::inkMid);
        g.drawText (note, juce::Rectangle<float> (200.0f, 10.0f).withPosition (ax + 8.0f, railY + 8.0f),
                    juce::Justification::centredLeft);
    }

    // --- Rail wet vers le sommateur de mix ------------------------------------
    g.setColour (palette::ink.withAlpha (0.9f));
    drawWeightedLine (g, { { vcfX1, railY }, { mixX - 9.0f, railY } }, mixV);
    g.setFont (fonts::lettering (9.0f));
    g.setColour (palette::inkMid);
    g.drawText ("WET", juce::Rectangle<float> (30.0f, 10.0f).withPosition (mixX - 52.0f, railY - 14.0f),
                juce::Justification::centredRight);

    drawSummingNode (g, { mixX, railY }, 8.0f);
    g.drawText ("MIX", juce::Rectangle<float> (30.0f, 10.0f).withPosition (mixX + 12.0f, railY - 20.0f),
                juce::Justification::centredLeft);

    // --- Sortie ------------------------------------------------------------------
    g.setColour (palette::ink);
    g.drawLine (mixX + 8.0f, railY, outX - 3.0f, railY, 1.4f);
    g.fillEllipse (outX - 3.0f, railY - 3.0f, 6.0f, 6.0f);                   // borne OUT
    g.setColour (palette::inkMid);
    g.drawText ("OUT", juce::Rectangle<float> (28.0f, 10.0f).withPosition (outX - 24.0f, railY - 17.0f),
                juce::Justification::centredRight);

    // --- Legende de figure ----------------------------------------------------
    const juce::String cap = juce::String ("FIG. 2 - SIGNAL PATH, ")
        + (pedal ? "LFO-CONTROLLED" : "ENVELOPE-CONTROLLED") + " VCF, "
        + (pedal ? "PEDAL" : (modeV == 0 ? "UP" : "DOWN")) + " MODE";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
