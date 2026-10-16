#pragma once
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "a11y/A11yStrings.h"
#include "a11y/ScreenReaderAnnouncer.h"

// ========================================================================================================
// TELESCOPE's accessible controls for the screen reader and the keyboard.
//
// The whole plugin UI is painted, so NVDA sees no controls in it. These components lie ON TOP of the
// painted UI (the mouse goes through them to the old interface) and give the screen reader real controls
// with focus, names and keys. They only paint the focus frame.
// ========================================================================================================
namespace telescope::a11yui
{
// Value change steps: normal, coarse and "to the limit" (Alt+Home/End).
inline constexpr int kStep    = 1;
inline constexpr int kCoarse  = 10;
inline constexpr int kExtreme = 1000;

// Where the grid gets its rows. Groups are "pages" (readings, bands, settings), rows are "Name, value".
struct GridSource
{
    virtual ~GridSource() = default;
    virtual int          groupCount() const = 0;
    virtual juce::String groupName (int group) const = 0;
    virtual int          rowCount (int group) const = 0;
    virtual juce::String rowName (int group, int row) const = 0;
    virtual juce::String rowValue (int group, int row) const = 0;
    virtual bool         isAdjustable (int, int) const { return false; }
    // step: +-kStep, +-kCoarse or +-kExtreme. true if the value changed (false at the edge of the range: silence).
    virtual bool         adjust (int, int, int) { return false; }
    // Button row (Reset, Load...): Enter runs the action. Returns a phrase about the result (may be empty,
    // e.g. when a dialog opens: the screen reader announces it by itself).
    virtual bool         isActivatable (int, int) const { return false; }
    virtual juce::String activate (int, int) { return {}; }
    // F1 on this row: what it is for. Empty means the general help of the control.
    virtual juce::String rowHelp (int, int) const { return {}; }
    // Periscope: spoken on the Q key (empty: the key searches by first letter as usual).
    virtual juce::String rowQ (int, int) const { return {}; }
};

// Shared part: live name for the screen reader, focus frame, Tab and F1.
class FocusOverlay : public juce::Component
{
public:
    std::function<void (bool forward)> onTab;
    std::function<void()>              onEscape;
    juce::String                       helpText;   // F1; English key of the translation catalog
    // Sets the speech language of this plugin window (see A11yStrings.h): called before every key and
    // every name request from the screen reader.
    std::function<void()>              syncLanguage;

    void sync() const { if (syncLanguage) syncLanguage(); }

    FocusOverlay()
    {
        setWantsKeyboardFocus (true);
        // The mouse goes through the control to the painted UI: sighted users work as before.
        setInterceptsMouseClicks (false, false);
    }

    // Text the screen reader reads as the control name (NVDA+Up arrow). Computed on every request,
    // so it is always fresh and needs no events.
    virtual juce::String liveTitle() const = 0;
    // The description is read after the name and role when focus arrives: room for context ("Spectrum, Readings").
    virtual juce::String liveDescription() const { return {}; }

    void say (const juce::String& text, a11y::SpeechQueue q = a11y::SpeechQueue::replacePrevious)
    {
        a11y::announce (*this, text, q);
    }

    void paint (juce::Graphics& g) override
    {
        if (! hasKeyboardFocus (false)) return;
        g.setColour (juce::Colours::orange);
        g.drawRect (getLocalBounds(), 2);
    }

    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override   { repaint(); }

protected:
    // Common keys. true if the key was handled here.
    bool handleCommonKey (const juce::KeyPress& key)
    {
        const auto mods = key.getModifiers();
        const int  code = key.getKeyCode();
        const bool noCtrlAlt = ! mods.isCtrlDown() && ! mods.isAltDown() && ! mods.isCommandDown();

        if (code == juce::KeyPress::tabKey && noCtrlAlt && onTab) { onTab (! mods.isShiftDown()); return true; }
        if (code == juce::KeyPress::F1Key && ! mods.isAnyModifierKeyDown() && helpText.isNotEmpty())
        {
            say (a11ytext::T (helpText.toRawUTF8()), a11y::SpeechQueue::keepAll);
            return true;
        }
        if (code == juce::KeyPress::escapeKey && ! mods.isAnyModifierKeyDown() && onEscape) { onEscape(); return true; }
        return false;
    }

    // What to do with a key the control does not know. Ctrl/Alt combinations and F keys go to the DAW
    // (Ctrl+S saves the project, Alt+F4 closes the window). Space goes to the DAW too: transport start and stop is
    // needed while listening to the analysis. Other bare keys are swallowed: otherwise arrows, Delete or letters
    // would trigger REAPER actions.
    static bool swallowUnknown (const juce::KeyPress& key)
    {
        const auto mods = key.getModifiers();
        const int  code = key.getKeyCode();
        if (mods.isCtrlDown() || mods.isAltDown() || mods.isCommandDown()) return false;
        if (code == juce::KeyPress::spaceKey || code == juce::KeyPress::escapeKey) return false;
        if (code >= juce::KeyPress::F1Key && code <= juce::KeyPress::F16Key) return false;
        return true;
    }

    std::unique_ptr<juce::AccessibilityHandler> makeHandler (juce::AccessibilityRole role,
                                                             juce::AccessibilityActions actions = {})
    {
        class LiveTitleHandler final : public juce::AccessibilityHandler
        {
        public:
            LiveTitleHandler (FocusOverlay& o, juce::AccessibilityRole r, juce::AccessibilityActions a)
                : juce::AccessibilityHandler (o, r, std::move (a)), owner (o) {}
            juce::String getTitle() const override       { owner.sync(); return owner.liveTitle(); }
            juce::String getDescription() const override { owner.sync(); return owner.liveDescription(); }
        private:
            FocusOverlay& owner;
        };
        return std::make_unique<LiveTitleHandler> (*this, role, std::move (actions));
    }
};

// ========================================================================================================
// A "groups x rows" grid in ONE focusable control (the LSampler-24 model):
//   Up/Down arrows: rows of the group; Left/Right: neighbouring group; Home/End: edge of the group;
//   Ctrl+Home/End: start/end of the whole grid; Page Up/Down: 10 rows;
//   Alt+Up/Down arrow: change the value by a step; Alt+Page Up/Down: coarse step;
//   Alt+Home/End: minimum/maximum; plus and minus: a value step; Enter on an adjustable row: edit
//   mode (plain arrows change the value, Enter or Esc leave it), on a button row: press it,
//   on other rows: read again; F5: read the row again with a fresh value;
//   a letter or digit: next row starting with it (Shift: backwards); F1: help of the row if it has one,
//   otherwise of the control; Esc: back.
// Edges are silent, no wrapping around.
// ========================================================================================================
class A11yGrid : public FocusOverlay
{
public:
    std::function<void (int group, int row)> onCursorChanged;   // the user moved the cursor
    std::function<void()>                    onEnter;           // if not set, Enter reads the row again
    juce::String emptyText;   // empty means "Empty" in the plugin language

    explicit A11yGrid (juce::AccessibilityRole r = juce::AccessibilityRole::list) : role (r) {}

    void setSource (GridSource* s) { source = s; clampCursor(); }

    int group() const noexcept { return grp; }
    int row()   const noexcept { return rw; }

    juce::String rowText (int g, int r) const
    {
        if (source == nullptr) return empty();
        const auto name  = source->rowName (g, r);
        const auto value = source->rowValue (g, r);
        return value.isEmpty() ? name : name + ", " + value;
    }

    juce::String currentText() const
    {
        if (source == nullptr || source->groupCount() <= 0 || source->rowCount (grp) <= 0) return empty();
        return rowText (grp, rw);
    }

    juce::String liveTitle() const override { return currentText(); }

    std::function<juce::String()> contextName;   // e.g. the lens name: "Spectrum"

    juce::String liveDescription() const override
    {
        juce::StringArray parts;
        if (contextName) parts.add (contextName());
        if (source != nullptr && source->groupCount() > 1) parts.add (source->groupName (grp));
        if (source != nullptr && source->rowCount (grp) > 0 && source->isAdjustable (grp, rw))
            parts.add (a11ytext::T ("plus or minus to change"));
        parts.removeEmptyStrings();
        return parts.joinIntoString (", ");
    }

    // Sets the cursor from code (silently if speak = false).
    void setCursor (int g, int r, bool speak, bool withGroup = false)
    {
        editing = false;
        grp = g; rw = r;
        clampCursor();
        repaint();
        if (speak) speakCurrent (withGroup);
    }

    // The source changed (another lens): keep the cursor in range and say nothing.
    void refresh() { clampCursor(); repaint(); }

    void speakCurrent (bool withGroup)
    {
        if (source == nullptr) return;
        say (withGroup && source->groupCount() > 1 ? source->groupName (grp) + ". " + currentText()
                                                   : currentText());
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        sync();
        if (editing && source != nullptr && handleEditKey (key)) return true;
        if (key.getKeyCode() == juce::KeyPress::F1Key && ! key.getModifiers().isAnyModifierKeyDown()
            && source != nullptr && source->rowCount (grp) > 0)
        {
            const auto help = source->rowHelp (grp, rw);
            if (help.isNotEmpty()) { say (help, a11y::SpeechQueue::keepAll); return true; }
        }
        if (handleCommonKey (key)) return true;
        if (source == nullptr) return swallowUnknown (key);

        const auto mods  = key.getModifiers();
        const int  code  = key.getKeyCode();
        const bool plain = ! mods.isAnyModifierKeyDown();
        const bool ctrl  = mods.isCtrlDown() && ! mods.isAltDown() && ! mods.isShiftDown();
        const bool alt   = mods.isAltDown() && ! mods.isCtrlDown() && ! mods.isShiftDown();
        const int  rows  = source->rowCount (grp);
        const int  groups = source->groupCount();

        // Plus and minus (main keyboard and numpad) change the value by a step. The character comes from
        // getTextCharacter: on the main keyboard "+" is Shift+"=", so "=" counts as plus too.
        if (! mods.isCtrlDown() && ! mods.isAltDown() && ! mods.isCommandDown())
        {
            const auto ch = key.getTextCharacter();
            if (code == juce::KeyPress::numberPadAdd || ch == '+' || ch == '=')      { applyStep (kStep);  return true; }
            if (code == juce::KeyPress::numberPadSubtract || ch == '-' || ch == '_') { applyStep (-kStep); return true; }
        }

        if (plain)
        {
            if (code == juce::KeyPress::upKey)       { moveTo (grp, rw - 1, false); return true; }
            if (code == juce::KeyPress::downKey)     { moveTo (grp, rw + 1, false); return true; }
            if (code == juce::KeyPress::pageUpKey)   { moveTo (grp, juce::jmax (0, rw - 10), false); return true; }
            if (code == juce::KeyPress::pageDownKey) { moveTo (grp, juce::jmin (rows - 1, rw + 10), false); return true; }
            if (code == juce::KeyPress::homeKey)     { moveTo (grp, 0, false); return true; }
            if (code == juce::KeyPress::endKey)      { moveTo (grp, rows - 1, false); return true; }
            if (code == juce::KeyPress::leftKey)     { if (grp > 0) moveTo (grp - 1, rw, true); return true; }
            if (code == juce::KeyPress::rightKey)    { if (grp + 1 < groups) moveTo (grp + 1, rw, true); return true; }
            if (code == juce::KeyPress::returnKey)
            {
                if (rows > 0 && source->isActivatable (grp, rw))
                {
                    const auto message = source->activate (grp, rw);
                    if (message.isNotEmpty()) say (message, a11y::SpeechQueue::keepAll);
                }
                else if (rows > 0 && source->isAdjustable (grp, rw))
                {
                    // Edit mode: plain arrows now change the value, Enter or Esc leave it.
                    editing = true;
                    say (a11ytext::T ("Edit"));
                }
                else if (onEnter) onEnter();
                else speakCurrent (false);
                return true;
            }
            if (code == juce::KeyPress::F5Key)       { speakCurrent (false); return true; }
        }

        if (ctrl && code == juce::KeyPress::homeKey) { moveTo (0, 0, true); return true; }
        if (ctrl && code == juce::KeyPress::endKey)
        {
            moveTo (groups - 1, source->rowCount (groups - 1) - 1, true);
            return true;
        }

        // Periscope: Ctrl+C copies the current row, Ctrl+Shift+C the whole group (its name, then one row per line), as
        // the text that is spoken. A screen reader user can paste readings into notes or a chat instead of retyping
        // them; a sighted user would take a screenshot. Letters arrive as upper-case key codes.
        if ((code == 'C' || code == 'c') && mods.isCtrlDown() && ! mods.isAltDown())
        {
            if (rows <= 0) return true;
            if (mods.isShiftDown())
            {
                juce::StringArray lines;
                if (groups > 1) lines.add (source->groupName (grp));
                for (int r = 0; r < rows; ++r) lines.add (rowText (grp, r));
                juce::SystemClipboard::copyTextToClipboard (lines.joinIntoString ("\r\n"));
                say (a11ytext::TF ("Group copied, {0} rows", { juce::String (rows) }));
            }
            else
            {
                juce::SystemClipboard::copyTextToClipboard (currentText());
                say (a11ytext::T ("Copied"));
            }
            return true;
        }

        if (alt)
        {
            int step = 0;
            if (code == juce::KeyPress::upKey)            step = kStep;
            else if (code == juce::KeyPress::downKey)     step = -kStep;
            else if (code == juce::KeyPress::pageUpKey)   step = kCoarse;
            else if (code == juce::KeyPress::pageDownKey) step = -kCoarse;
            else if (code == juce::KeyPress::endKey)      step = kExtreme;
            else if (code == juce::KeyPress::homeKey)     step = -kExtreme;

            if (step != 0) { applyStep (step); return true; }
        }

        // Periscope: Q on a row that has one (a Spectrum peak or dip) speaks its EQ Q. The key code, not the character,
        // so the same physical key works in a Russian layout; on other rows Q stays a first-letter search.
        if (plain && (code == 'Q' || code == 'q') && rows > 0)
        {
            const auto q = source->rowQ (grp, rw);
            if (q.isNotEmpty()) { say (q); return true; }
        }

        // Search by the first character of the name. getTextCharacter, not the key code: otherwise arrows and Page Up
        // fall into the "printable" code range (a known JUCE trap).
        if (! mods.isCtrlDown() && ! mods.isAltDown() && ! mods.isCommandDown())
        {
            auto typed = key.getTextCharacter();
            if (mods.isShiftDown() && ((code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9')))
                typed = (juce::juce_wchar) code;   // Shift+digit gives "!": take the digit itself
            if (typed >= 33 && typed != 127)
            {
                searchByInitial (typed, mods.isShiftDown() ? -1 : 1);
                return true;
            }
        }

        return swallowUnknown (key);
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return makeHandler (role);
    }

    void focusLost (FocusChangeType t) override { editing = false; FocusOverlay::focusLost (t); }

private:
    // Changes the value of the current row. If it changed, speak only the new value; at the edge, silence.
    void applyStep (int step)
    {
        if (source == nullptr || source->rowCount (grp) <= 0) return;
        if (! source->isAdjustable (grp, rw)) { say (a11ytext::T ("Read only")); return; }
        if (source->adjust (grp, rw, step)) say (source->rowValue (grp, rw));
    }

    // Edit mode keys. false means the key goes on by the normal rules (Tab, F1, plus and minus).
    bool handleEditKey (const juce::KeyPress& key)
    {
        const auto mods  = key.getModifiers();
        const int  code  = key.getKeyCode();
        if (code == juce::KeyPress::tabKey) { editing = false; return false; }
        if (mods.isAnyModifierKeyDown()) return false;

        if (code == juce::KeyPress::returnKey || code == juce::KeyPress::escapeKey)
        {
            editing = false;
            say (a11ytext::T ("Done"));
            return true;
        }
        if (code == juce::KeyPress::upKey)       { applyStep (kStep);     return true; }
        if (code == juce::KeyPress::downKey)     { applyStep (-kStep);    return true; }
        if (code == juce::KeyPress::pageUpKey)   { applyStep (kCoarse);   return true; }
        if (code == juce::KeyPress::pageDownKey) { applyStep (-kCoarse);  return true; }
        if (code == juce::KeyPress::endKey)      { applyStep (kExtreme);  return true; }
        if (code == juce::KeyPress::homeKey)     { applyStep (-kExtreme); return true; }
        if (code == juce::KeyPress::F5Key)       { say (source->rowValue (grp, rw)); return true; }
        if (code == juce::KeyPress::leftKey || code == juce::KeyPress::rightKey) return true;   // stay on the row
        return false;
    }

    juce::String empty() const { return emptyText.isNotEmpty() ? emptyText : a11ytext::T ("Empty"); }

    void clampCursor()
    {
        const int groups = source != nullptr ? source->groupCount() : 0;
        grp = groups > 0 ? juce::jlimit (0, groups - 1, grp) : 0;
        const int rows = source != nullptr && groups > 0 ? source->rowCount (grp) : 0;
        rw = rows > 0 ? juce::jlimit (0, rows - 1, rw) : 0;
    }

    void moveTo (int g, int r, bool withGroup)
    {
        editing = false;
        const int oldG = grp, oldR = rw;
        grp = g; rw = r;
        clampCursor();
        if (grp == oldG && rw == oldR) return;   // edge: silence
        repaint();
        if (onCursorChanged) onCursorChanged (grp, rw);
        speakCurrent (withGroup && grp != oldG);
    }

    void searchByInitial (juce::juce_wchar initial, int direction)
    {
        const int rows = source->rowCount (grp);
        if (rows <= 0) return;
        const auto target = juce::CharacterFunctions::toLowerCase (initial);
        for (int distance = 1; distance <= rows; ++distance)
        {
            int index = (rw + direction * distance) % rows;
            if (index < 0) index += rows;
            const auto name = source->rowName (grp, index).trimStart();
            if (name.isNotEmpty() && juce::CharacterFunctions::toLowerCase (name[0]) == target)
            {
                moveTo (grp, index, false);
                return;
            }
        }
        // No match: silence, as at the edges.
    }

    juce::AccessibilityRole role;
    GridSource* source = nullptr;
    int grp = 0, rw = 0;
    bool editing = false;   // edit mode (Enter on an adjustable row)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (A11yGrid)
};

// ========================================================================================================
// A button over a painted row. Enter or Space presses it. Live name: "Language, English".
// ========================================================================================================
class A11yButton : public FocusOverlay
{
public:
    std::function<juce::String()> getTitleText;
    std::function<void()>         onPress;

    juce::String liveTitle() const override { return getTitleText ? getTitleText() : juce::String(); }

    void press() { if (onPress) onPress(); }

    bool keyPressed (const juce::KeyPress& key) override
    {
        sync();
        if (handleCommonKey (key)) return true;
        const auto mods = key.getModifiers();
        const int  code = key.getKeyCode();
        if (! mods.isAnyModifierKeyDown() && (code == juce::KeyPress::returnKey || code == juce::KeyPress::spaceKey))
        {
            press();
            return true;
        }
        return swallowUnknown (key);
    }

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        // Press action: NVDA can press the button with its own command, without plugin keys.
        return makeHandler (juce::AccessibilityRole::button,
                            juce::AccessibilityActions().addAction (juce::AccessibilityActionType::press,
                                                                    [this] { press(); }));
    }
};
}
