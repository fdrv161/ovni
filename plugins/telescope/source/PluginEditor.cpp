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

// Имена линз для речи. Обычный регистр, а не КАПС с экрана: короткие слова капсом некоторые синтезаторы
// читают по буквам.
constexpr const char* kSpokenLensNames[kNumLenses] = {
    "Loudness", "Dynamics", "Spectrum", "Spectrogram", "Waterfall", "CQT", "Spiral",
    "Scope", "Band correlation", "Stereo spectrogram", "Field", "Tonal balance", "Verdict"
};
}

// ================== ДОСТУПНОСТЬ: источники строк для невидимых элементов ==================
struct TelescopeEditor::LensListSource final : a11yui::GridSource
{
    int          groupCount() const override           { return 1; }
    juce::String groupName (int) const override         { return "Lenses"; }
    int          rowCount (int) const override          { return kNumLenses; }
    juce::String rowName (int, int row) const override  { return kSpokenLensNames[juce::jlimit (0, kNumLenses - 1, row)]; }
    juce::String rowValue (int, int) const override     { return {}; }
};

// Показания текущей линзы. Линза меняется при каждом выборе, поэтому источник спрашивает её заново
// при каждом запросе, а не хранит указатель.
struct TelescopeEditor::LensPanelSource final : a11yui::GridSource
{
    explicit LensPanelSource (TelescopeEditor& e) : ed (e) {}

    Lens* lens() const { return ed.lens.get(); }
    bool  hasReadings() const { auto* l = lens(); return l != nullptr && l->a11yGroupCount() > 0; }

    int          groupCount() const override        { return hasReadings() ? lens()->a11yGroupCount() : 1; }
    juce::String groupName (int g) const override   { return hasReadings() ? lens()->a11yGroupName (g) : juce::String ("Readings"); }
    int          rowCount (int g) const override    { return hasReadings() ? lens()->a11yRowCount (g) : 1; }
    juce::String rowName (int g, int r) const override
    {
        // Честно: у этой линзы ещё нет текстовых показаний (делаются по очереди).
        return hasReadings() ? lens()->a11yRowName (g, r) : juce::String ("No text readings for this lens yet");
    }
    juce::String rowValue (int g, int r) const override     { return hasReadings() ? lens()->a11yRowValue (g, r) : juce::String(); }
    bool isAdjustable (int g, int r) const override         { return hasReadings() && lens()->a11yIsAdjustable (g, r); }
    bool adjust (int g, int r, int step) override           { return hasReadings() && lens()->a11yAdjust (g, r, step); }
    bool isActivatable (int g, int r) const override        { return hasReadings() && lens()->a11yIsActivatable (g, r); }
    juce::String activate (int g, int r) override           { return hasReadings() ? lens()->a11yActivate (g, r) : juce::String(); }

    TelescopeEditor& ed;
};

// ================== ПАНЕЛЬ «PLUGIN»: шапка, язык, тема ==================
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
    juce::String groupName (int) const override { return "Plugin"; }
    int          rowCount (int) const override  { return kRows; }

    juce::String rowName (int, int r) const override
    {
        static const char* const names[kRows] = { "Bypass", "Preset", "Save preset", "A B slot",
                                                  "Copy this slot to the other", "Window size", "Language", "Theme" };
        return names[juce::jlimit (0, kRows - 1, r)];
    }

    juce::String rowValue (int, int r) const override
    {
        auto& p = ed.proc;
        switch (r)
        {
            case rowBypass: return isBypassed() ? "On" : "Off";
            case rowPreset:
            {
                const auto c = p.presets().current();
                const auto n = c.name.isEmpty() ? juce::String ("Init") : c.name;
                return c.modified ? n + ", modified" : n;
            }
            case rowAb:       return juce::String::charToString ((juce::juce_wchar) p.ab().activeSlot());
            case rowSize:
            {
                static const char* const sizes[] = { "Small", "Medium", "Large" };
                return sizes[zoomIndex (ed.getZoom())];
            }
            case rowLanguage: return strings::endonymOf (strings::languageOf (p.apvts.state));
            case rowTheme:    return look::theme() == look::Theme::light ? "Light" : "Dark";
            default:          return {};
        }
    }

    bool isAdjustable (int, int r) const override
    {
        return r == rowBypass || r == rowPreset || r == rowAb || r == rowSize || r == rowLanguage || r == rowTheme;
    }

    // Размер окна меняется только шагами (Enter на нём — режим правки), остальное Enter нажимает.
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
                if ((p.ab().activeSlot() == 'B') == up) return false;   // плюс — B, минус — A
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
                return true;
            }
            case rowTheme:
                if ((look::theme() == look::Theme::light) == up) return false;   // плюс — светлая
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
            case rowBypass:   setBypassed (! isBypassed()); return "Bypass " + rowValue (0, rowBypass);
            case rowPreset:   ed.showPresetMenuA11y(); return {};
            case rowSave:     ed.showSaveDialogA11y(); return {};
            case rowAb:       p.ab().toggle(); ed.repaint(); return "Slot " + rowValue (0, rowAb);
            case rowAbCopy:
            {
                const auto from = rowValue (0, rowAb);
                p.ab().copyActiveToOther();
                ed.repaint();
                return "Copied " + from + " to " + (from == "A" ? "B" : "A");
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
            // Жест begin/end: хост (REAPER, запись автоматизации) видит одно касание параметра, а не «рывок».
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 ((float) i));
            param->endChangeGesture();
        }
    };

    strip.onLanguage = [this] (const juce::String& code)
    {
        strings::setLanguage (proc.apvts.state, code);
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

    // Доступность: картинка линзы — только для глаз, её цифры читает панель показаний поверх неё.
    // Список и панель следуют за линзой, откуда бы ни пришла смена (клик, автоматизация, пресет), молча.
    lens->setAccessible (false);
    lens->setMouseClickGrabsKeyboardFocus (false);   // см. setupAccessibility: клик мышью не забирает клавиатуру
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

// ================== ДОСТУПНОСТЬ ==================
void TelescopeEditor::setupAccessibility()
{
    setTitle ("TELESCOPE");
    // Нарисованная полоса линз — только для глаз: всё, что в ней есть, дают элементы ниже.
    strip.setAccessible (false);

    // Для зрячих всё как было: щелчок мышью по нарисованному НЕ переводит клавиатурный фокус на невидимые
    // элементы. Иначе JUCE поднимался бы от места щелчка к холсту и отдавал фокус списку линз, и стрелки,
    // буквы, Delete после клика уходили бы в плагин, а не в REAPER. Холст — единственный прямой ребёнок
    // редактора (см. PluginEditorBase).
    strip.setMouseClickGrabsKeyboardFocus (false);
    for (auto* child : getChildren())
        child->setMouseClickGrabsKeyboardFocus (false);

    lensListSource  = std::make_unique<LensListSource>();
    lensPanelSource = std::make_unique<LensPanelSource> (*this);

    a11yLensList.setSource (lensListSource.get());
    a11yLensList.helpText = "Lens list. Up and Down arrows choose a lens, a letter jumps to a lens by name. "
                            "Enter: go to the readings of the lens. Tab: next control. "
                            "Space starts and stops the host transport.";
    // Стрелка в списке сразу включает линзу: так же, как клик по строке полосы.
    a11yLensList.onCursorChanged = [this] (int, int row) { if (strip.onSelect) strip.onSelect (row); };
    a11yLensList.onEnter = [this] { focusA11y (a11yPanel); };

    a11yPanel.setSource (lensPanelSource.get());
    a11yPanel.helpText = "Readings of the current lens. Up and Down arrows: rows. Left and Right arrows: groups. "
                         "Plus or minus: change a setting or move the probe. "
                         "Enter on a setting: edit mode, then Up and Down arrows change it, Page Up and Page Down "
                         "make a bigger step, Enter or Escape finish. Enter on an action: do it. "
                         "Alt plus arrows also change a value. F5: read the row again with a fresh value. "
                         "Escape: back to the lens list. Space starts and stops the host transport.";
    a11yPanel.contextName = [this]
    {
        return currentLens >= 0 ? juce::String (kSpokenLensNames[currentLens]) : juce::String();
    };
    a11yPanel.onEscape = [this] { focusA11y (a11yLensList); };
    a11yPanel.onCursorChanged = [this] (int g, int r)
    {
        if (currentLens >= 0) panelCursor[(size_t) currentLens] = { g, r };
    };

    pluginRowsSource = std::make_unique<PluginRowsSource> (*this);
    a11yPlugin.setSource (pluginRowsSource.get());
    a11yPlugin.contextName = [] { return juce::String ("Plugin"); };
    a11yPlugin.helpText = "Plugin controls: bypass, presets, A and B comparison, window size, language of the "
                          "on-screen labels and colour theme. Up and Down arrows: rows. Plus or minus: change. "
                          "Enter: switch, open the preset menu or the save dialog. Escape: back to the lens list.";
    a11yPlugin.onEscape = [this] { focusA11y (a11yLensList); };

    // Порядок Tab задан явно: JUCE иначе сортирует элементы по положению на экране.
    int order = 1;
    for (auto* c : { &a11yLensList, &a11yPanel, &a11yPlugin })
    {
        c->setExplicitFocusOrder (order++);
        c->onTab = [this, c] (bool forward) { cycleA11yFocus (*c, forward); };
        addToCanvas (*c);
    }
}

// Невидимые элементы лежат ровно поверх нарисованного: рамка фокуса совпадает с тем, что видит зрячий.
void TelescopeEditor::layoutAccessibility()
{
    const auto s = strip.getBounds();
    a11yLensList.setBounds (s.withHeight (juce::jmin (s.getHeight(), strip.listHeight())));
    a11yPanel.setBounds (lensArea);
    // Шапка — всё, что выше полосы линз.
    a11yPlugin.setBounds (0, 0, lensArea.getRight(), juce::jmax (1, s.getY()));
}

// Меню пресетов для клавиатуры: то же, что по клику на имя пресета, но открывается у панели и после
// закрытия возвращает фокус туда же (иначе фокус остаётся «нигде» и NVDA молчит).
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

    root.addSubMenu ("Production", prod);
    root.addSubMenu ("Sound design", sd);
    if (proc.presets().userPresets().size() > 0)
        root.addSubMenu ("User", user);

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
            // Возврат фокуса сам прочитает строку «Preset, <имя>»: отдельный анонс наложился бы на неё.
            safe->focusA11yLater (safe->a11yPlugin);
        });
}

void TelescopeEditor::showSaveDialogA11y()
{
    auto* w = new juce::AlertWindow ("Save preset", "Preset name:", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", "My Preset", "Preset name");
    if (auto* te = w->getTextEditor ("name")) te->setTitle ("Preset name");
    w->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
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

// Фокус после закрытия меню или диалога: с задержкой, чтобы JUCE и хост закончили свою смену фокуса.
void TelescopeEditor::focusA11yLater (juce::Component& target)
{
    juce::Timer::callAfterDelay (60, [safe = juce::Component::SafePointer<juce::Component> (&target), ed = juce::Component::SafePointer<TelescopeEditor> (this)]
    {
        if (safe != nullptr && ed != nullptr) ed->focusA11y (*safe);
    });
}

// Перенос фокуса по рецепту, проверенному в REAPER: клавиатурный фокус JUCE и отдельно фокус UIA,
// чтобы NVDA точно узнал о смене, даже если JUCE считает, что фокус не менялся.
void TelescopeEditor::focusA11y (juce::Component& target)
{
    if (! target.isShowing()) return;
    target.grabKeyboardFocus();
    if (auto* h = target.getAccessibilityHandler()) h->grabFocus();
}

// Tab и Shift+Tab ходят по кругу между тремя панелями (по кругу — как принято для Tab).
void TelescopeEditor::cycleA11yFocus (juce::Component& from, bool forward)
{
    const std::array<juce::Component*, 3> order { &a11yLensList, &a11yPanel, &a11yPlugin };
    const int n = (int) order.size();
    int i = 0;
    while (i < n && order[(size_t) i] != &from) ++i;
    i = (i + (forward ? 1 : n - 1)) % n;
    focusA11y (*order[(size_t) i]);
}

// Если хост отдал фокус самому окну, а не элементу, NVDA молчал бы: переводим фокус на список линз.
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
