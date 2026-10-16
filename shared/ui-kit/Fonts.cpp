#include "Fonts.h"
#include <juce_events/juce_events.h>   // juce::DeletedAtShutdown
#include "OvniUikitData.h"   // generado por juce_add_binary_data(ovni_uikit_assets) — ver CMakeLists.txt

namespace ovni::ui::fonts
{
namespace
{
    juce::Typeface::Ptr loadTf (const char* data, int size)
    {
        if (data == nullptr || size <= 0)
            return nullptr;
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }

    // Periscope (audit M1): the typefaces used to be function-local statics, so their DirectWrite and Direct2D
    // factories were released by the C runtime at DLL unload, under the Windows loader lock (the class of bug
    // that hung REAPER with the Panel and Glow images). Now they live in a DeletedAtShutdown object: JUCE frees
    // it when the last plugin instance shuts JUCE down, on a normal thread. If that never happens (the host
    // exits with the plugin still loaded), the object is simply never destroyed, which is harmless; the only
    // static left is a raw pointer, with nothing to run at unload. Fonts are made on the message thread only.
    struct FontCache : juce::DeletedAtShutdown
    {
        juce::Typeface::Ptr clash      = loadTf (OvniUikitData::ClashGroteskSemibold_ttf, OvniUikitData::ClashGroteskSemibold_ttfSize);
        juce::Typeface::Ptr general    = loadTf (OvniUikitData::GeneralSansRegular_ttf, OvniUikitData::GeneralSansRegular_ttfSize);
        juce::Typeface::Ptr generalMed = loadTf (OvniUikitData::GeneralSansMedium_ttf, OvniUikitData::GeneralSansMedium_ttfSize);
        juce::Typeface::Ptr jbMono     = loadTf (OvniUikitData::JetBrainsMonoRegular_ttf, OvniUikitData::JetBrainsMonoRegular_ttfSize);

        ~FontCache() override { instance() = nullptr; }

        static FontCache*& instance() { static FontCache* p = nullptr; return p; }
        static FontCache& get()
        {
            auto& p = instance();
            if (p == nullptr) p = new FontCache();
            return *p;
        }
    };

    const juce::Typeface::Ptr& clash()      { return FontCache::get().clash; }
    const juce::Typeface::Ptr& general()    { return FontCache::get().general; }
    const juce::Typeface::Ptr& generalMed() { return FontCache::get().generalMed; }
    const juce::Typeface::Ptr& jbMono()     { return FontCache::get().jbMono; }

    juce::Font make (const juce::Typeface::Ptr& tf, float height, bool boldFallback)
    {
        // FontOptions (tf) y no FontOptions().withTypeface (tf): el default trae estilo "Regular", y JUCE 8 afirma
        // (juce_FontOptions.h) que con un typeface el estilo tiene que venir vacío. En Release no se nota; en Debug
        // era un jassert por fuente creada (hallazgo de OJOS etapa 4, prompt 77 §6).
        if (tf != nullptr)
            return juce::Font (juce::FontOptions (tf).withHeight (height));
        // fallback: system sans (no se debería llegar acá con las fuentes embebidas)
        auto opts = juce::FontOptions().withHeight (height);
        return juce::Font (boldFallback ? opts.withStyle ("Bold") : opts);
    }
}

juce::Font display (float h) { return make (clash(),      h, true);  }
juce::Font body    (float h) { return make (general(),    h, false); }
juce::Font label   (float h) { return make (generalMed(), h, false); }
juce::Font mono    (float h) { return make (jbMono(),     h, false); }
}
