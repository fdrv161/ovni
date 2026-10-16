#pragma once
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>

// Comparador A/B (heredado de ÓRBITA M5): dos snapshots del estado de parámetros en memoria. toggle()
// guarda el slot activo y carga el otro -> comparás dos sonidos al instante. copyActiveToOther() arranca
// el otro desde el activo (alt+click). No toca disco.
namespace ovni::presets
{
class ABState
{
public:
    explicit ABState (juce::AudioProcessorValueTreeState& s) : apvts (s)
    {
        a = apvts.copyState();
        b = apvts.copyState();
    }

    void toggle();                  // guarda el slot activo, carga el otro
    // Periscope (audit M3): both slots from the current state, A active. Called after the host restores a
    // project; until the first toggle the other slot starts as a copy of the active one, so the first A/B press
    // never brings back the bare state of construction time (English, no reference, default lens settings).
    void resync()                   { a = apvts.copyState(); b = a.createCopy(); onA = true; fresh = true; }
    // Called after a slot replaced the whole state, so the plugin can push it to its engine.
    std::function<void()> onStateReplaced;
    void copyActiveToOther();       // alt+click: arrancar el otro desde el activo
    char activeSlot() const { return onA ? 'A' : 'B'; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::ValueTree a, b;
    bool onA = true;
    bool fresh = true;   // no toggle since construction or resync()
};
}
