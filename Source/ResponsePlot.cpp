#include "ResponsePlot.h"
#include "ManualStyle.h"
#include "WahEngine.h"

#include <complex>

namespace wahmxa
{

namespace
{
    constexpr float kFreqMin = 20.0f;
    constexpr float kFreqMax = 20000.0f;
    constexpr float kDbTop   = 18.0f;
    constexpr float kDbBot   = -42.0f;
    constexpr int   kPoints  = 180;

    float xForFreq (juce::Rectangle<float> r, float f)
    {
        const float t = std::log (f / kFreqMin) / std::log (kFreqMax / kFreqMin);
        return r.getX() + t * r.getWidth();
    }

    float yForDb (juce::Rectangle<float> r, float db)
    {
        const float t = (kDbTop - db) / (kDbTop - kDbBot);
        return r.getY() + t * r.getHeight();
    }

    float freqForX (juce::Rectangle<float> r, float x)
    {
        const float t = (x - r.getX()) / r.getWidth();
        return kFreqMin * std::pow (kFreqMax / kFreqMin, t);
    }

    juce::String freqLabel (float f)
    {
        if (f >= 1000.0f)
            return juce::String (f / 1000.0f, 0) + "k";
        return juce::String ((int) f);
    }

    // Module de la reponse totale : (1-mix) + mix x pic x H_bp(s), avec
    // H_bp = (s/Q)/(s^2 + s/Q + 1), s = j f/fc — le prototype analogique du
    // SVF, pic unite a fc, rehausse par peakGainFor. Meme algebre que le moteur.
    float responseDb (float freqHz, float fc, float q, float peak, float mixAmt)
    {
        const std::complex<float> s (0.0f, freqHz / fc);
        const std::complex<float> h = (s / q) / (s * s + s / q + 1.0f);
        const std::complex<float> total = (1.0f - mixAmt) + mixAmt * peak * h;
        return 20.0f * std::log10 (std::abs (total) + 1.0e-9f);
    }
}

ResponsePlot::ResponsePlot (WahProcessor& proc)
    : processor (proc)
{
    auto& apvts = processor.getAPVTS();
    freq = apvts.getRawParameterValue ("freq");
    res  = apvts.getRawParameterValue ("res");
    mode = apvts.getRawParameterValue ("mode");
    mix  = apvts.getRawParameterValue ("mix");

    setInterceptsMouseClicks (false, false);
}

void ResponsePlot::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();
    auto caption = full.removeFromBottom (16.0f);
    auto box = full;

    double fs = processor.getSampleRate();
    if (fs <= 0.0)
        fs = 48000.0;

    const float baseV = freq->load();
    const float resV  = res->load();
    const int   modeV = juce::roundToInt (mode->load());
    const float mixV  = mix->load();

    const float sweep = processor.getSweep01();
    float fc = processor.getFcHz();
    if (fc <= 0.0f)
        fc = WahEngine::fcFor (baseV, modeV != 1, 0.0f, fs);

    const float q    = WahEngine::qFor (resV);
    const float peak = WahEngine::peakGainFor (resV);

    // --- Grille ------------------------------------------------------------
    g.setColour (palette::inkFaint);
    static const float gridFreqs[] = { 50.0f, 100.0f, 200.0f, 500.0f,
                                       1000.0f, 2000.0f, 5000.0f, 10000.0f };
    for (const float f : gridFreqs)
        g.drawVerticalLine ((int) xForFreq (box, f), box.getY() + 1.0f, box.getBottom() - 1.0f);

    static const float gridDbs[] = { 12.0f, 0.0f, -12.0f, -24.0f, -36.0f };
    for (const float db : gridDbs)
    {
        g.setColour (db == 0.0f ? palette::inkMid.withAlpha (0.65f) : palette::inkFaint);
        g.drawHorizontalLine ((int) yForDb (box, db), box.getX() + 1.0f, box.getRight() - 1.0f);
    }

    // Bande de balayage reelle : bornes fc(0) et fc(1), en tirete spot.
    {
        const float f0 = WahEngine::fcFor (baseV, true, 0.0f, fs);
        const float f1 = WahEngine::fcFor (baseV, true, 1.0f, fs);
        for (const float f : { f0, f1 })
        {
            juce::Path lineP, dashed;
            const float sx = xForFreq (box, f);
            lineP.startNewSubPath (sx, box.getY() + 1.0f);
            lineP.lineTo (sx, box.getBottom() - 1.0f);
            const float dashes[] = { 2.0f, 4.0f };
            juce::PathStrokeType (0.8f).createDashedStroke (dashed, lineP, dashes, 2);
            g.setColour (palette::spot.withAlpha (0.35f));
            g.fillPath (dashed);
        }
    }

    // Repere du balayage : index vertical a la frequence centrale courante.
    {
        const float sx = xForFreq (box, fc);
        g.setColour (palette::spot.withAlpha (0.30f));
        g.drawVerticalLine ((int) sx, box.getY() + 1.0f, box.getBottom() - 1.0f);

        juce::Path idx;   // petit index triangulaire en haut
        idx.addTriangle (sx - 3.5f, box.getY() + 1.0f, sx + 3.5f, box.getY() + 1.0f, sx, box.getY() + 7.0f);
        g.setColour (palette::spot);
        g.fillPath (idx);
    }

    // --- Courbe ------------------------------------------------------------
    {
        juce::Path p;
        for (int i = 0; i < kPoints; ++i)
        {
            const float px = box.getX() + (float) i / (float) (kPoints - 1) * box.getWidth();
            const float f  = freqForX (box, px);
            const float db = juce::jlimit (kDbBot, kDbTop, responseDb (f, fc, q, peak, mixV));
            const float py = yForDb (box, db);
            if (i == 0) p.startNewSubPath (px, py);
            else        p.lineTo (px, py);
        }
        g.setColour (palette::spot);
        g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
    }

    // --- Echelles ----------------------------------------------------------
    // Chaque chiffre est pose sur un cartouche film : ni le grain ni la grille
    // ne peuvent corrompre une valeur que le regard doit pouvoir croire.
    auto drawFigure = [&g] (const juce::String& text, juce::Point<float> anchor,
                            juce::Justification just)
    {
        const auto font = fonts::mono (9.0f);
        const float tw  = juce::GlyphArrangement::getStringWidth (font, text);
        auto area = juce::Rectangle<float> (tw + 6.0f, 11.0f).withCentre (anchor);
        if (just.testFlags (juce::Justification::left))
            area.setX (anchor.x - 3.0f);
        else if (just.testFlags (juce::Justification::right))
            area.setX (anchor.x - tw - 3.0f);

        g.setColour (palette::film);
        g.fillRect (area);
        g.setFont (font);
        g.setColour (palette::inkMid);
        g.drawText (text, area, juce::Justification::centred);
    };

    for (const float f : gridFreqs)
        drawFigure (freqLabel (f), { xForFreq (box, f), box.getBottom() - 8.0f },
                    juce::Justification::centred);

    for (const float db : gridDbs)
        drawFigure ((db > 0.0f ? "+" : "") + juce::String ((int) db),
                    { box.getX() + 6.0f, yForDb (box, db) - 6.0f },
                    juce::Justification::left);

    // Designation des unites, une fois par echelle, convention de plan.
    drawFigure ("dB", { box.getX() + 6.0f, box.getY() + 10.0f }, juce::Justification::left);
    drawFigure ("Hz", { box.getRight() - 6.0f, box.getBottom() - 8.0f }, juce::Justification::right);

    // Tallies : frequence centrale et position de balayage reelles.
    {
        const juce::String fcText = fc >= 1000.0f
            ? juce::String (fc / 1000.0f, 2) + " kHz"
            : juce::String ((int) fc) + " Hz";

        auto tally = juce::Rectangle<float> (120.0f, 12.0f)
                         .withPosition (box.getRight() - 126.0f, box.getY() + 6.0f);
        g.setColour (palette::film);
        g.fillRect (tally.expanded (3.0f, 1.0f));
        g.setFont (fonts::mono (10.0f));
        g.setColour (palette::inkMid);
        g.drawText ("fc", tally.removeFromLeft (20.0f), juce::Justification::centredLeft);
        g.setColour (palette::ink);
        g.drawText (fcText, tally, juce::Justification::centredLeft);

        auto tally2 = juce::Rectangle<float> (120.0f, 12.0f)
                          .withPosition (box.getRight() - 126.0f, box.getY() + 20.0f);
        g.setColour (palette::film);
        g.fillRect (tally2.expanded (3.0f, 1.0f));
        g.setFont (fonts::mono (10.0f));
        g.setColour (palette::inkMid);
        g.drawText ("swp", tally2.removeFromLeft (36.0f), juce::Justification::centredLeft);
        g.setColour (palette::ink);
        g.drawText (juce::String (juce::roundToInt (sweep * 100.0f)) + " %",
                    tally2, juce::Justification::centredLeft);
    }

    // --- Cadre + legende de figure ------------------------------------------
    g.setColour (palette::ink);
    g.drawRect (box, 1.0f);

    const juce::String cap = "FIG. 1 - FREQUENCY RESPONSE AT LIVE SWEEP POSITION";
    g.setFont (fonts::lettering (10.0f));
    g.setColour (palette::inkMid);
    g.drawText (cap, caption.withTrimmedTop (4.0f), juce::Justification::bottomLeft);

    const float capW = juce::GlyphArrangement::getStringWidth (fonts::lettering (10.0f), cap);
    g.setColour (palette::inkFaint);
    g.drawHorizontalLine ((int) (caption.getBottom() - 3.0f),
                          caption.getX() + capW + 10.0f, caption.getRight());
}

}
