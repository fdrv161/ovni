#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Speaks messages for NVDA, JAWS, Narrator (Windows, via UI Automation)
// and VoiceOver (macOS). Details and rules: references/announcements-and-focus.md.
namespace a11y
{
// How to treat the screen reader's speech queue.
enum class SpeechQueue
{
    replacePrevious, // a new message interrupts the previous one: values, navigation
    keepAll          // the message must not be lost: result of an operation, an error
};

// Speaks text. source is a visible component of the plugin window that is in the accessibility
// tree: the focused control or a permanent status label. Message thread only.
// Does NOT change the source's title/description, so temporary text does not stick.
void announce (juce::Component& source, const juce::String& text,
               SpeechQueue queue = SpeechQueue::replacePrevious);

// For telemetry and status: does not repeat text that was just spoken.
// Do not use it for responses to key presses: the user may want to hear the phrase again.
class DedupAnnouncer
{
public:
    void say (juce::Component& source, const juce::String& text,
              SpeechQueue queue = SpeechQueue::replacePrevious)
    {
        if (text == lastText)
            return;
        lastText = text;
        announce (source, text, queue);
    }

    void reset() { lastText.clear(); }

private:
    juce::String lastText;
};
} // namespace a11y
