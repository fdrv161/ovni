#pragma once
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <juce_core/juce_core.h>

// ========================================================================================================
// Speech translations for the screen reader.
//
// The key is the English phrase; the table (A11yStringsTable.h) gives it in the other plugin languages. Phrases
// with numbers are templates with {0}, {1}...: word order differs between languages, so no gluing of fragments.
// No translation means English is spoken (like the on-screen labels, see strings::getFrom).
//
// Speech uses the screen language (the Language row). The editor sets it before every use of the strings
// (setLanguage): two plugin windows may use different languages while the translation functions are shared.
// Message thread only.
// ========================================================================================================
namespace telescope::a11ytext
{
// Column order of the table.
enum Lang { en = 0, es, pt, fr, de, it, ru, kNumLangs };

struct Row
{
    const char* text[kNumLangs];
};

inline const Row* rows() noexcept;   // A11yStringsTable.h
inline int        numRows() noexcept;

inline int& currentLanguage() noexcept
{
    static int lang = en;
    return lang;
}

inline void setLanguage (const juce::String& code)
{
    static const char* const codes[kNumLangs] = { "en", "es", "pt", "fr", "de", "it", "ru" };
    int lang = en;
    for (int i = 0; i < kNumLangs; ++i)
        if (code == codes[i]) lang = i;
    currentLanguage() = lang;
}

inline Lang language() noexcept { return (Lang) currentLanguage(); }

// Translates a phrase. Table strings are UTF-8.
inline juce::String T (const char* english)
{
    static const std::unordered_map<std::string, const Row*> index = []
    {
        std::unordered_map<std::string, const Row*> m;
        for (int i = 0; i < numRows(); ++i) m.emplace (rows()[i].text[en], &rows()[i]);
        return m;
    }();

    const int lang = currentLanguage();
    const auto it = index.find (english);
    if (it == index.end()) return juce::String::fromUTF8 (english);
    const char* t = it->second->text[lang];
    return juce::String::fromUTF8 (t != nullptr && *t != '\0' ? t : english);
}

// Translates a template, substituting {0}, {1}... in order.
inline juce::String TF (const char* englishTemplate, std::initializer_list<juce::String> args)
{
    auto s = T (englishTemplate);
    int i = 0;
    for (const auto& a : args)
        s = s.replace ("{" + juce::String (i++) + "}", a);
    return s;
}
}

#include "a11y/A11yStringsTable.h"
