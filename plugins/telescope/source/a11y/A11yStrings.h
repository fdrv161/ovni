#pragma once
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <juce_core/juce_core.h>

// ========================================================================================================
// Переводы речи для скринридера.
//
// Ключ — английская фраза; таблица ниже (A11yStringsTable.h) даёт её на остальных языках плагина. Фразы с
// числами — шаблоны с {0}, {1}…: в разных языках другой порядок слов, склеивать из кусочков нельзя.
// Нет перевода — звучит английский (как у подписей экрана, см. strings::getFrom).
//
// Язык речи — тот же, что у экрана (строка Language). Его выставляет редактор перед каждым обращением
// к строкам (setLanguage): у двух окон плагина могут быть разные языки, а функции перевода общие.
// Только поток сообщений.
// ========================================================================================================
namespace telescope::a11ytext
{
// Порядок колонок таблицы.
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

// Перевод фразы. Строки таблицы — UTF-8.
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

// Перевод шаблона с подстановкой {0}, {1}… по порядку.
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
