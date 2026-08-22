# WahMXA

An auto-wah with three drive modes: an envelope follower (up or down) or an automatic pedal LFO sweeps a resonant TPT state-variable band-pass over 2.5 octaves.

Audio plugin (AU / VST3 / Standalone) built with [JUCE](https://juce.com). Part of the MXA plugin suite. macOS 11+.

## Build

```sh
git clone --recurse-submodules <repo-url>
cd WahMXA
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

If you already have the MXA suite checked out with a shared `../JUCE` folder, the submodule is optional — the build falls back to the sibling folder automatically.
