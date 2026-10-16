#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "lenses/BandCorrelationLens.h"
#include "lenses/CqtLens.h"
#include "lenses/DynamicsLens.h"
#include "lenses/LoudnessLens.h"
#include "lenses/PlaceholderLens.h"
#include "lenses/ScopeLens.h"
#include "lenses/SpectrogramLens.h"
#include "lenses/SpectrumLens.h"
#include "lenses/SpiralLens.h"
#include "lenses/StereoSpectrogramLens.h"
#include "lenses/FieldLens.h"        // ===== 53 =====
#include "lenses/WaterfallLens.h"    // ===== 53 =====
// ===== 54: TONAL BALANCE (lente 12) =====
#include "lenses/TonalBalanceLens.h"
// ===== 55: VERDICT (lente 13) =====
#include "lenses/VerdictLens.h"

namespace telescope
{
namespace
{
constexpr int kStripWidth = 150;   // coords de DISEÑO; el zoom S/M/L lo escala solo

// Lens names for speech. Normal case, not the on-screen CAPS: some synthesizers spell short all-caps words
// letter by letter.
constexpr const char* kSpokenLensNames[kNumLenses] = {
    "Loudness", "Dynamics", "Spectrum", "Spectrogram", "Waterfall", "CQT", "Spiral",
    "Scope", "Band correlation", "Stereo spectrogram", "Field", "Tonal balance", "Verdict"
};
}

// ================== ACCESSIBILITY: row sources for the invisible controls ==================
struct TelescopeEditor::LensListSource final : a11yui::GridSource
{
    int          groupCount() const override           { return 1; }
    juce::String groupName (int) const override         { return a11ytext::T ("Lenses"); }
    int          rowCount (int) const override          { return kNumLenses; }
    juce::String rowName (int, int row) const override  { return a11ytext::T (kSpokenLensNames[juce::jlimit (0, kNumLenses - 1, row)]); }
    juce::String rowValue (int, int) const override     { return {}; }
};

// Readings of the current lens. The lens changes on every selection, so the source asks for it again
// on every request instead of keeping a pointer.
struct TelescopeEditor::LensPanelSource final : a11yui::GridSource
{
    explicit LensPanelSource (TelescopeEditor& e) : ed (e) {}

    Lens* lens() const { return ed.lens.get(); }
    bool  hasReadings() const { auto* l = lens(); return l != nullptr && l->a11yGroupCount() > 0; }

    int          groupCount() const override        { return hasReadings() ? lens()->a11yGroupCount() : 1; }
    juce::String groupName (int g) const override   { return hasReadings() ? lens()->a11yGroupName (g) : a11ytext::T ("Readings"); }
    int          rowCount (int g) const override    { return hasReadings() ? lens()->a11yRowCount (g) : 1; }
    juce::String rowName (int g, int r) const override
    {
        // Say it plainly: this lens has no text readings yet.
        return hasReadings() ? lens()->a11yRowName (g, r) : a11ytext::T ("No text readings for this lens yet");
    }
    juce::String rowValue (int g, int r) const override     { return hasReadings() ? lens()->a11yRowValue (g, r) : juce::String(); }
    bool isAdjustable (int g, int r) const override         { return hasReadings() && lens()->a11yIsAdjustable (g, r); }
    bool adjust (int g, int r, int step) override           { return hasReadings() && lens()->a11yAdjust (g, r, step); }
    bool isActivatable (int g, int r) const override        { return hasReadings() && lens()->a11yIsActivatable (g, r); }
    juce::String rowHelp (int g, int r) const override      { return hasReadings() ? lens()->a11yRowHelp (g, r) : juce::String(); }
    juce::String rowQ (int g, int r) const override         { return hasReadings() ? lens()->a11yRowQ (g, r) : juce::String(); }

    juce::String activate (int g, int r) override
    {
        if (! hasReadings()) return {};
        const auto message = lens()->a11yActivate (g, r);
        // The action moved something the cursor should follow (Spectrum: the probe jumps to a peak): go there
        // and read that row; a message on top of it would cut it off.
        int ng = 0, nr = 0;
        if (lens()->a11yTakeCursorRequest (ng, nr))
        {
            ed.a11yPanel.setCursor (ng, nr, true);
            if (ed.currentLens >= 0) ed.panelCursor[(size_t) ed.currentLens] = { ed.a11yPanel.group(), ed.a11yPanel.row() };
            return {};
        }
        return message;
    }

    TelescopeEditor& ed;
};

// ================== THE "PLUGIN" PANEL: header, language, theme ==================
struct TelescopeEditor::PluginRowsSource final : a11yui::GridSource
{
    enum Row { rowBypass = 0, rowPreset, rowSave, rowAb, rowAbCopy, rowSize, rowLanguage, rowTheme, kRows };

    explicit PluginRowsSource (TelescopeEditor& e) : ed (e) {}

    juce::RangedAudioParameter* bypassParam() const { return ed.proc.apvts.getParameter (ed.proc.bypassParamID()); }
    bool isBypassed() const { auto* p = bypassParam(); return p != nullptr && p->getValue() >= 0.5f; }

    void setBypassed (bool on)
    {
        if (auto* p = bypassParam())
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (on ? 1.0f : 0.0f);
            p->endChangeGesture();
        }
        ed.repaint();
    }

    using Zoom = ovni::PluginEditorBase::Zoom;
    static int zoomIndex (Zoom z) { return z == Zoom::small ? 0 : (z == Zoom::large ? 2 : 1); }

    int          groupCount() const override    { return 1; }
    juce::String groupName (int) const override { return a11ytext::T ("Plugin"); }
    int          rowCount (int) const override  { return kRows; }

    juce::String rowName (int, int r) const override
    {
        static const char* const names[kRows] = { "Bypass", "Preset", "Save preset", "A B slot",
                                                  "Copy this slot to the other", "Window size", "Language", "Theme" };
        return a11ytext::T (names[juce::jlimit (0, kRows - 1, r)]);
    }

    juce::String rowValue (int, int r) const override
    {
        auto& p = ed.proc;
        switch (r)
        {
            case rowBypass: return a11ytext::T (isBypassed() ? "On" : "Off");
            case rowPreset:
            {
                const auto c = p.presets().current();
                const auto n = c.name.isEmpty() ? juce::String ("Init") : c.name;
                return c.modified ? a11ytext::TF ("{0}, modified", { n }) : n;
            }
            case rowAb:       return juce::String::charToString ((juce::juce_wchar) p.ab().activeSlot());
            case rowSize:
            {
                static const char* const sizes[] = { "Small", "Medium", "Large" };
                return a11ytext::T (sizes[zoomIndex (ed.getZoom())]);
            }
            case rowLanguage: return strings::endonymOf (strings::languageOf (p.apvts.state));
            case rowTheme:    return a11ytext::T (look::theme() == look::Theme::light ? "Light" : "Dark");
            default:          return {};
        }
    }

    bool isAdjustable (int, int r) const override
    {
        return r == rowBypass || r == rowPreset || r == rowAb || r == rowSize || r == rowLanguage || r == rowTheme;
    }

    // Window size only changes in steps (Enter on it starts edit mode); Enter presses everything else.
    bool isActivatable (int, int r) const override { return r != rowSize; }

    bool adjust (int, int r, int step) override
    {
        const bool up = step > 0;
        auto& p = ed.proc;
        switch (r)
        {
            case rowBypass:
                if (isBypassed() == up) return false;
                setBypassed (up);
                return true;
            case rowPreset:
                if (up) p.presets().next(); else p.presets().prev();
                ed.repaint();
                return true;
            case rowAb:
                if ((p.ab().activeSlot() == 'B') == up) return false;   // plus is B, minus is A
                p.ab().toggle();
                ed.repaint();
                return true;
            case rowSize:
            {
                const int now = zoomIndex (ed.getZoom());
                const int next = juce::jlimit (0, 2, now + (up ? 1 : -1));
                if (next == now) return false;
                ed.setZoom (next == 0 ? Zoom::small : (next == 2 ? Zoom::large : Zoom::medium));
                return true;
            }
            case rowLanguage:
            {
                const auto codes = strings::availableLanguages();
                const int now  = juce::jmax (0, codes.indexOf (strings::languageOf (p.apvts.state)));
                const int next = juce::jlimit (0, codes.size() - 1, now + (up ? 1 : -1));
                if (next == now) return false;
                strings::setLanguage (p.apvts.state, codes[next]);
                ThemePreference::setValue (ThemePreference::kLanguageKey, codes[next]);   // Periscope: for new instances
                return true;
            }
            case rowTheme:
                if ((look::theme() == look::Theme::light) == up) return false;   // plus is light
                ed.toggleTheme();
                return true;
            default: return false;
        }
    }

    juce::String activate (int, int r) override
    {
        auto& p = ed.proc;
        switch (r)
        {
            case rowBypass:   setBypassed (! isBypassed()); return a11ytext::T (isBypassed() ? "Bypass on" : "Bypass off");
            case rowPreset:   ed.showPresetMenuA11y(); return {};
            case rowSave:     ed.showSaveDialogA11y(); return {};
            case rowAb:       p.ab().toggle(); ed.repaint(); return a11ytext::TF ("Slot {0}", { rowValue (0, rowAb) });
            case rowAbCopy:
            {
                const auto from = rowValue (0, rowAb);
                p.ab().copyActiveToOther();
                ed.repaint();
                return a11ytext::TF ("Copied {0} to {1}", { from, from == "A" ? "B" : "A" });
            }
            case rowLanguage: ed.strip.cycleLanguage(); return rowValue (0, rowLanguage);
            case rowTheme:    ed.toggleTheme(); return rowValue (0, rowTheme);
            default:          return {};
        }
    }

    TelescopeEditor& ed;
};


// Las lentes CONSTRUIDAS. Las que todavía no existen caen en PlaceholderLens: un panel con su nombre y
// nada más (ni "coming soon" ni promesas — ver PlaceholderLens.h).
std::unique_ptr<Lens> TelescopeEditor::makeLens (LensId id)
{
    switch (id)
    {
        case LensId::loudness: return std::make_unique<LoudnessLens> (proc);
        case LensId::dynamics: return std::make_unique<DynamicsLens> (proc);
        case LensId::scope:    return std::make_unique<ScopeLens> (proc);
        case LensId::spectrum:    return std::make_unique<SpectrumLens> (proc);
        case LensId::spectrogram: return std::make_unique<SpectrogramLens> (proc);
        case LensId::cqt:      return std::make_unique<CqtLens> (proc);
        case LensId::spiral:   return std::make_unique<SpiralLens> (proc);
        case LensId::bandCorrelation:   return std::make_unique<BandCorrelationLens> (proc);
        case LensId::stereoSpectrogram: return std::make_unique<StereoSpectrogramLens> (proc);
        // ===== 53: las dos 3D (proyección 2.5D por software, D-46) =====
        case LensId::waterfall: return std::make_unique<WaterfallLens> (proc);
        case LensId::field:     return std::make_unique<FieldLens> (proc);
        // ===== 54: TONAL BALANCE =====
        case LensId::tonalBalance:      return std::make_unique<TonalBalanceLens> (proc);
        // ===== 55: VERDICT — la unica que no muestra: dice =====
        case LensId::verdict:           return std::make_unique<VerdictLens> (proc);
        default:               return std::make_unique<PlaceholderLens> (id);
    }
}

// Verde espectral: TELESCOPE mide el espectro y el espacio, no los mueve (familia Espectral del sello).
TelescopeEditor::TelescopeEditor (TelescopeProcessor& p)
    : ovni::PluginEditorBase (p, juce::String::fromUTF8 ("ANA\xc2\xb7" "01")), proc (p)
{
    // F2 de la 0.2: el tema es la preferencia del usuario (OVNI.settings), leída al abrir la ventana.
    look::setTheme (ThemePreference::load());
    setFamilyHue (look::ink().green);

    proc.editorOpened();   // ver editorOpened/editorClosed: la máscara se apaga cuando se cierra la última

    addToCanvas (strip);
    setupAccessibility();
    // Las lentes CONSTRUIDAS. Con VERDICT (55) están LAS TRECE: setBuilt reemplaza la máscara entera.
    strip.setBuilt ((1u << (int) LensId::loudness)
                  | (1u << (int) LensId::dynamics)
                  | (1u << (int) LensId::spectrum)
                  | (1u << (int) LensId::spectrogram)
                  | (1u << (int) LensId::waterfall)
                  | (1u << (int) LensId::cqt)
                  | (1u << (int) LensId::spiral)
                  | (1u << (int) LensId::scope)
                  | (1u << (int) LensId::bandCorrelation)
                  | (1u << (int) LensId::stereoSpectrogram)
                  | (1u << (int) LensId::field)
                  | (1u << (int) LensId::tonalBalance)
                  | (1u << (int) LensId::verdict));

    strip.onSelect = [this] (int i)
    {
        if (auto* param = proc.apvts.getParameter ("lens"))
        {
            // Begin/end gesture: the host (REAPER, automation recording) sees one touch of the parameter, not a jump.
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 ((float) i));
            param->endChangeGesture();
        }
    };

    strip.onLanguage = [this] (const juce::String& code)
    {
        strings::setLanguage (proc.apvts.state, code);
        ThemePreference::setValue (ThemePreference::kLanguageKey, strings::languageOf (proc.apvts.state));   // Periscope
    };

    strip.onTheme = [this] { toggleTheme(); };
    themeListener.fn = [this] { applyTheme(); };
    ThemePreference::changed().addChangeListener (&themeListener);

    proc.apvts.addParameterListener ("lens", this);
    proc.apvts.state.addListener (this);
    applyLanguage();

    // El VALOR DEL PARÁMETRO, no getParameterAsValue(): el APVTS vuelca los parámetros al ValueTree en
    // diferido (por timer), así que si el host ya dejó `lens` en SCOPE antes de abrir la ventana, el árbol
    // todavía dice LOUDNESS y el editor abriría la lente equivocada. El raw value es sincrónico.
    const auto* lensValue = proc.apvts.getRawParameterValue ("lens");
    showLens (lensValue != nullptr ? juce::roundToInt (lensValue->load()) : 0);

   #if TELESCOPE_HAS_EYEPIECE_INTRO
    // T8 (F4 de la 0.2): la primera vez que se abre el editor con la 0.2, la tarjeta que presenta a EYEPIECE.
    if (EyepieceIntro::shouldShow())
    {
        intro = std::make_unique<EyepieceIntroCard> (strings::languageOf (proc.apvts.state));
        addToCanvas (*intro);
    }
   #endif

    setBaseSize (980, 620);
    applyTheme();
}

// Al cerrar la ventana, lo que la lente encendió se APAGA: el motor vuelve a los módulos siempre-activos
// (kAlwaysOnModules) en cuanto se cierra el ÚLTIMO editor. Sin esto, cerrar el plugin con el espectro a la
// vista dejaba la FFT corriendo para siempre sin nadie mirando (HIGH del revisor del 50). El conteo lo
// lleva el processor: con dos ventanas abiertas, cerrar una no le apaga el módulo a la otra.
TelescopeEditor::~TelescopeEditor()
{
    // Periscope (audit H1): the lens goes first, while the accessible grids and their sources still exist; a
    // lens being destroyed can pump the message queue (an open file dialog) and run pending async calls.
    a11yPoll.stopTimer();
    lens.reset();
    ThemePreference::changed().removeChangeListener (&themeListener);
    proc.apvts.state.removeListener (this);
    proc.apvts.removeParameterListener ("lens", this);
    proc.editorClosed();
}

void TelescopeEditor::parameterChanged (const juce::String& id, float value)
{
    if (id != "lens") return;
    // El callback del APVTS puede venir de cualquier hilo (automatización del host): al message thread.
    const int index = juce::roundToInt (value);
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<TelescopeEditor> (this), index]
                                     { if (safe != nullptr) safe->showLens (index); });
}

void TelescopeEditor::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& prop)
{
    if (prop.toString() == strings::kLanguageProperty) applyLanguageSafely();
}

// 56c — EL ÁRBOL ENTERO CAMBIÓ DE LUGAR. `apvts.replaceState (tree)` —lo que corre cuando el host
// restaura la sesión— hace `state = newState` (JUCE 8.0.13,
// juce_AudioProcessorValueTreeState.cpp:400), y `ValueTree::operator=` avisa a los listeners por ACÁ, no
// por valueTreePropertyChanged: no hay ninguna propiedad que cambie, cambia el árbol al que se apunta.
// Sin esto, con la ventana abierta el idioma guardado en la sesión no llegaba nunca a la pantalla — el
// árbol decía "de" y la tira seguía en inglés. Es el mismo camino, porque el idioma vigente se vuelve a
// leer del árbol (que ya es el nuevo).
void TelescopeEditor::valueTreeRedirected (juce::ValueTree&)
{
    applyLanguageSafely();
}

// 56c — AL MESSAGE THREAD, COMO parameterChanged. `juce::ValueTree::Listener` notifica de forma SÍNCRONA
// y en el hilo que hizo el `setProperty`, y `applyLanguage()` toca la tira y pide repaint. Un preset
// cargado desde el hilo de automatización del host entraba por acá y tocaba componentes desde ese hilo.
// Cuando ya estamos en el message thread se aplica en el acto: pasar todo por callAsync metería un frame
// de retraso en el caso común (el clic en el chip de la tira) sin ganar nada.
void TelescopeEditor::applyLanguageSafely()
{
    if (juce::MessageManager::existsAndIsCurrentThread()) { applyLanguage(); return; }

    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<TelescopeEditor> (this)]
                                     { if (safe != nullptr) safe->applyLanguage(); });
}

// La tira Y la lente visible. La lente no "sabe" el idioma: lo lee del árbol cada vez que pinta, así
// que basta con pedirle que pinte de nuevo — y hay que pedírselo, porque su repaint está suspendido
// mientras el dibujo no cambia (VisualizerBase pausa en reposo).
void TelescopeEditor::applyLanguage()
{
    appliedOnMessageThread.store (juce::MessageManager::existsAndIsCurrentThread());
    applyCount.fetch_add (1);
    strip.setLanguage (strings::languageOf (proc.apvts.state));
    a11ytext::setLanguage (strings::languageOf (proc.apvts.state));   // speech uses the same language
    if (lens != nullptr) lens->repaint();
   #if TELESCOPE_HAS_EYEPIECE_INTRO
    if (intro != nullptr) { intro->setLanguage (strings::languageOf (proc.apvts.state)); placeIntro(); }
   #endif
}

void TelescopeEditor::showLens (int index)
{
    index = juce::jlimit (0, kNumLenses - 1, index);
    if (index == currentLens) return;
    currentLens = index;

    lens = makeLens ((LensId) index);

    addToCanvas (*lens);
    lens->setPixelAnchor (this);   // T2: el editor es el que el host pone en un píxel entero (ver Lens.h)
    lens->setBounds (lensArea);

    // Accessibility: the lens picture is for the eyes only; the readings panel on top of it reads its numbers.
    // The list and the panel follow the lens silently, wherever the change came from (click, automation, preset).
    lens->setAccessible (false);
    lens->setMouseClickGrabsKeyboardFocus (false);   // see setupAccessibility: a mouse click does not take the keyboard
    a11yLensList.setCursor (0, index, false);
    a11yPanel.setCursor (panelCursor[(size_t) index].first, panelCursor[(size_t) index].second, false);
    a11yPanel.toFront (false);

    // LENTE A DEMANDA: corren los módulos que la lente visible necesita MÁS los que no se apagan nunca
    // (ver kAlwaysOnModules en analysis/AnalysisFrame.h: el medidor de loudness acumula, no puede tener
    // agujeros porque el usuario se fue a mirar el espectro un rato).
    proc.setEnabledModules (lens->requiredModules() | kAlwaysOnModules);
    strip.setSelected (index);
   #if TELESCOPE_HAS_EYEPIECE_INTRO
    if (intro != nullptr) intro->toFront (false);   // T8: la tarjeta queda arriba de la lente nueva
   #endif
    applyLanguage();
}

// Reconstruir la lente es lo mismo que elegirla otra vez: sus capas horneadas (la estática, las cachés
// raster) nacen con la tinta nueva. La historia vive en el processor, así que no se pierde nada.
void TelescopeEditor::rebuildLens()
{
    const int index = currentLens;
    currentLens = -1;
    lens.reset();
    showLens (juce::jmax (0, index));
}

// El MARCO. En oscuro, FrameInk por defecto: el marco del sello tal cual, bit por bit. En claro: la base
// lisa (el Panel del sello es una atmósfera oscura y no tiene versión clara), las tintas del papel, sin el
// glow del nombre (sobre papel se ve como una mancha), y el texto del slot A/B encendido en papel sobre el
// verde oscuro.
void TelescopeEditor::applyTheme()
{
    const auto& k = look::ink();
    setFamilyHue (k.green);

    FrameInk f;
    if (look::theme() == look::Theme::light)
    {
        f.base       = k.bg0;
        f.atmosphere = false;
        f.sheen      = juce::Colours::white.withAlpha (0.55f);
        f.topLine    = juce::Colours::white.withAlpha (0.80f);
        f.lineSoft   = k.lineSoft;
        f.txt        = k.txt;
        f.mut        = k.mut;
        f.fnt        = k.mut;        // la insignia ANA·01: en papel, el secundario (el terciario no llega a 4.5:1)
        f.onHue      = k.bg1;
        f.bezel      = k.line;
        f.brackets   = k.txt.withAlpha (0.28f);
        f.nameGlow   = false;
    }
    setFrameInk (f);

    rebuildLens();
    strip.repaint();
   #if TELESCOPE_HAS_EYEPIECE_INTRO
    if (intro != nullptr) intro->repaint();
   #endif
    repaint();
}

void TelescopeEditor::toggleTheme()
{
    const auto next = look::theme() == look::Theme::light ? look::Theme::dark : look::Theme::light;
    look::setTheme (next);
    ThemePreference::save (next);
    // Síncrono: el clic ya está en el message thread, y así la ventana que se clickeó cambia en este cuadro.
    ThemePreference::changed().sendSynchronousChangeMessage();
}

void TelescopeEditor::layoutBody (juce::Rectangle<int> body)
{
    body.reduce (ovni::ui::theme::padIn, ovni::ui::theme::padIn / 2);

    strip.setBounds (body.removeFromLeft (kStripWidth));
    body.removeFromLeft (ovni::ui::theme::padIn);

    lensArea = body;
    if (lens != nullptr) lens->setBounds (lensArea);
   #if TELESCOPE_HAS_EYEPIECE_INTRO
    placeIntro();
   #endif
    layoutAccessibility();
}

// ================== ACCESSIBILITY ==================
void TelescopeEditor::setupAccessibility()
{
    setTitle ("Periscope");   // the fork's name; the painted header and the author's texts keep TELESCOPE
    // The painted lens strip is for the eyes only: the controls below provide everything in it.
    strip.setAccessible (false);

    // Sighted users get the old behaviour: a mouse click on the painted UI does NOT move keyboard focus to the
    // invisible controls. Otherwise JUCE would walk up from the click to the canvas and give focus to the lens list,
    // and arrows, letters and Delete after a click would go to the plugin instead of REAPER. The canvas is the only
    // direct child of the editor (see PluginEditorBase).
    strip.setMouseClickGrabsKeyboardFocus (false);
    for (auto* child : getChildren())
        child->setMouseClickGrabsKeyboardFocus (false);

    lensListSource  = std::make_unique<LensListSource>();
    lensPanelSource = std::make_unique<LensPanelSource> (*this);

    a11yLensList.setSource (lensListSource.get());
    a11yLensList.helpText = "Lens list. Up and Down arrows choose a lens, a letter jumps to a lens by name. "
                            "Enter: go to the readings of the lens. Tab: next control. "
                            "Space starts and stops the host transport.";
    // An arrow in the list switches the lens at once, like a click on a row of the strip.
    a11yLensList.onCursorChanged = [this] (int, int row) { if (strip.onSelect) strip.onSelect (row); };
    a11yLensList.onEnter = [this] { focusA11y (a11yPanel); };

    a11yPanel.setSource (lensPanelSource.get());
    a11yPanel.helpText = "Readings of the current lens. Up and Down arrows: rows. Left and Right arrows: groups. "
                         "Plus or minus: change a setting or move the probe. "
                         "Enter on a setting: edit mode, then Up and Down arrows change it, Page Up and Page Down "
                         "make a bigger step, Enter or Escape finish. Enter on an action: do it. "
                         "Alt plus arrows also change a value. F5: read the row again with a fresh value. "
                         "Some rows have their own help on F1. "
                         "Escape: back to the lens list. Space starts and stops the host transport.";
    a11yPanel.contextName = [this]
    {
        return currentLens >= 0 ? a11ytext::T (kSpokenLensNames[currentLens]) : juce::String();
    };
    a11yPanel.onEscape = [this] { focusA11y (a11yLensList); };
    a11yPanel.onCursorChanged = [this] (int g, int r)
    {
        if (currentLens >= 0) panelCursor[(size_t) currentLens] = { g, r };
    };

    pluginRowsSource = std::make_unique<PluginRowsSource> (*this);
    a11yPlugin.setSource (pluginRowsSource.get());
    a11yPlugin.contextName = [] { return a11ytext::T ("Plugin"); };
    a11yPlugin.helpText = "Plugin controls: bypass, presets, A and B comparison, window size, language "
                          "and colour theme. Up and Down arrows: rows. Plus or minus: change. "
                          "Enter: switch, open the preset menu or the save dialog. Escape: back to the lens list.";
    a11yPlugin.onEscape = [this] { focusA11y (a11yLensList); };

    a11yPoll.fn = [this] { pollLensEvents(); };
    a11yPoll.startTimerHz (4);

    // Explicit Tab order: otherwise JUCE sorts the controls by their position on screen.
    int order = 1;
    for (auto* c : { &a11yLensList, &a11yPanel, &a11yPlugin })
    {
        c->syncLanguage = [this] { a11ytext::setLanguage (strings::languageOf (proc.apvts.state)); };
        c->setExplicitFocusOrder (order++);
        c->onTab = [this, c] (bool forward) { cycleA11yFocus (*c, forward); };
        addToCanvas (*c);
    }
}

// The invisible controls sit exactly over the painted UI, so the focus frame matches what a sighted user sees.
void TelescopeEditor::layoutAccessibility()
{
    const auto s = strip.getBounds();
    a11yLensList.setBounds (s.withHeight (juce::jmin (s.getHeight(), strip.listHeight())));
    a11yPanel.setBounds (lensArea);
    // The header is everything above the lens strip.
    a11yPlugin.setBounds (0, 0, lensArea.getRight(), juce::jmax (1, s.getY()));
}

// Preset menu for the keyboard: the same as clicking the preset name, but it opens at the panel and gives focus
// back there when it closes (otherwise focus stays nowhere and NVDA is silent).
void TelescopeEditor::showPresetMenuA11y()
{
    namespace pr = ovni::presets;
    juce::PopupMenu prod, sd, user, root;
    const auto& all = pr::factoryPresets();
    for (int i = 0; i < (int) all.size(); ++i)
    {
        auto& sub = (all[(size_t) i].category == pr::Category::Production) ? prod : sd;
        sub.addItem (i + 1, juce::String::fromUTF8 (all[(size_t) i].name));
    }
    int uid = 10001;
    for (auto& f : proc.presets().userPresets())
        user.addItem (uid++, f.getFileNameWithoutExtension());

    root.addSubMenu (a11ytext::T ("Production"), prod);
    root.addSubMenu (a11ytext::T ("Sound design"), sd);
    if (proc.presets().userPresets().size() > 0)
        root.addSubMenu (a11ytext::T ("User"), user);

    root.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&a11yPlugin),
        [safe = juce::Component::SafePointer<TelescopeEditor> (this)] (int r)
        {
            if (safe == nullptr) return;
            auto& pm = safe->proc.presets();
            if (r >= 1 && r <= (int) pr::factoryPresets().size())
                pm.applyFactory (r - 1);
            else if (r >= 10001)
            {
                const auto users = pm.userPresets();
                const int idx = r - 10001;
                if (idx >= 0 && idx < users.size()) pm.applyUserFile (users[idx]);
            }
            safe->repaint();
            // Returning focus reads the "Preset, <name>" row by itself: a separate announcement would overlap it.
            safe->focusA11yLater (safe->a11yPlugin);
        });
}

void TelescopeEditor::showSaveDialogA11y()
{
    auto* w = new juce::AlertWindow (a11ytext::T ("Save preset"), a11ytext::T ("Preset name:"), juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", a11ytext::T ("My Preset"), a11ytext::T ("Preset name"));
    if (auto* te = w->getTextEditor ("name")) te->setTitle (a11ytext::T ("Preset name"));
    w->addButton (a11ytext::T ("Save"),   1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton (a11ytext::T ("Cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create (
        [safe = juce::Component::SafePointer<TelescopeEditor> (this), w] (int res)
        {
            if (safe == nullptr) return;
            if (res == 1)
            {
                const auto nm = w->getTextEditorContents ("name").trim();
                if (nm.isNotEmpty()) safe->proc.presets().saveUser (nm);
            }
            safe->repaint();
            safe->focusA11yLater (safe->a11yPlugin);
        }), true);   // deleteWhenDismissed
}

// A lens message is spoken from the invisible control that has focus (sounds more natural), otherwise from the
// readings panel: the event source must be a visible control of the window, or NVDA cannot tie it to the window.
void TelescopeEditor::pollLensEvents()
{
    if (lens == nullptr || ! isShowing()) return;
    a11ytext::setLanguage (strings::languageOf (proc.apvts.state));
    const auto message = lens->a11yPollEvent();
    if (message.isEmpty()) return;

    juce::Component* source = &a11yPanel;
    for (auto* c : { &a11yLensList, &a11yPanel, &a11yPlugin })
        if (c->hasKeyboardFocus (false)) source = c;
    a11y::announce (*source, message, a11y::SpeechQueue::keepAll);
}

// Focus after a menu or dialog closes: delayed, so JUCE and the host finish their own focus change first.
void TelescopeEditor::focusA11yLater (juce::Component& target)
{
    juce::Timer::callAfterDelay (60, [safe = juce::Component::SafePointer<juce::Component> (&target), ed = juce::Component::SafePointer<TelescopeEditor> (this)]
    {
        if (safe != nullptr && ed != nullptr) ed->focusA11y (*safe);
    });
}

// Focus move recipe tested in REAPER: JUCE keyboard focus and, separately, UIA focus, so that NVDA
// learns about the change even when JUCE thinks focus did not change.
void TelescopeEditor::focusA11y (juce::Component& target)
{
    if (! target.isShowing()) return;
    target.grabKeyboardFocus();
    if (auto* h = target.getAccessibilityHandler()) h->grabFocus();
}

// Tab and Shift+Tab cycle through the three panels (wrapping around, as usual for Tab).
void TelescopeEditor::cycleA11yFocus (juce::Component& from, bool forward)
{
    const std::array<juce::Component*, 3> order { &a11yLensList, &a11yPanel, &a11yPlugin };
    const int n = (int) order.size();
    int i = 0;
    while (i < n && order[(size_t) i] != &from) ++i;
    i = (i + (forward ? 1 : n - 1)) % n;
    focusA11y (*order[(size_t) i]);
}

// If the host gives focus to the window itself rather than a control, NVDA would be silent: move it to the lens list.
void TelescopeEditor::focusGained (FocusChangeType)
{
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<TelescopeEditor> (this)]
    {
        if (safe != nullptr && safe->hasKeyboardFocus (false)) safe->focusA11y (safe->a11yLensList);
    });
}

#if TELESCOPE_HAS_EYEPIECE_INTRO
// T8: abajo a la derecha de la lente, con el margen del sello. El ancho es fijo (el texto se lee en un bloque) y
// el alto sale del texto envuelto en ese idioma.
void TelescopeEditor::placeIntro()
{
    if (intro == nullptr || lensArea.isEmpty()) return;
    const int pad = ovni::ui::theme::padIn;
    const int w = juce::jmin (380, lensArea.getWidth() - 2 * pad);
    const int h = intro->heightForWidth (w);
    intro->setBounds (lensArea.getRight() - pad - w, lensArea.getBottom() - pad - h, w, h);
    intro->toFront (false);
}
#endif

void TelescopeEditor::pumpLensFrames (int n)
{
    if (lens != nullptr) lens->pumpFrames (n);
}
}
