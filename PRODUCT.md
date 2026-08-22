# Product

<!-- impeccable:product-schema 1 -->

## Platform

desktop

Native JUCE audio plugin (AU/VST3/Standalone, macOS 11+). Sibling of the MXA suite; family design authority: `../PhaserMXA/DESIGN.md`; family product context: `../PhaserMXA/PRODUCT.md`.

## Product Purpose

An auto-wah with three drive modes: an envelope follower (up or down) or an automatic pedal LFO sweeps a resonant TPT state-variable band-pass over 2.5 octaves.

## Capabilities and Constraints

- Exactly six parameters: `sens` (env modes: rectified-signal gain up to +36 dB; pedal mode: pedal travel), `speed` (env modes: attack 25→3 ms, release 400→60 ms; pedal mode: pedal rate 0.1–8 Hz), `freq` (sweep base 150–800 Hz), `res` (Q 1.5–10, peak lift √Q), `mode` (Up/Down/Pedal choice), `mix`.
- Pedal mode: raised-cosine rocking LFO (`pedalValueFor`, `pedalRateHzFor`), heel at 0, toe at 1, sweeping the UP direction.
- Filter: TPT SVF band-pass, normalized (bp × 1/Q) with √Q peak lift; mono envelope drives both channels coherently; sweep slewed ~5 ms against zipper (`WahEngine`).
- UI truth taps: atomic live sweep position (`uiSweep`) and swept frequency (`uiFcHz`); static `fcFor()` / `qFor()` / `peakGainFor()` / `attackMsFor()` / `releaseMsFor()` — the single source of truth for FIG. 1's response curve (analog SVF prototype, mix included, drawn at the live sweep position) and FIG. 2's printed values; FIG. 2's control wire thickness IS the live envelope.
- Editor: Service Manual family sheet, 820×470, spot ink glacier-cyan #5FD4E0, DWG NO. MXA-WH-01.

## Brand Commitments

Inherits the family's: MXAudio, "BY MESCALINA" credit, one spot ink per sibling (Wah = glacier-cyan).

## Evidence on Hand

Working DSP (`Source/WahEngine.*`). No users/testimonials — nothing may be fabricated.
