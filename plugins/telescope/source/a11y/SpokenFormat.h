#pragma once
#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
#include "a11y/A11yStrings.h"
#include "lenses/NoteName.h"

// ========================================================================================================
// Числа для речи: коротко, без лишних нулей, единицы словами, которые синтезатор читает одинаково.
// Только ASCII в шаблонах английского: символы вроде «·», «¢», «∞» синтезаторы читают по-разному.
// Слова и порядок слов — из каталога переводов (A11yStrings.h), на языке, выбранном в плагине.
// ========================================================================================================
namespace telescope::spoken
{
using a11ytext::T;
using a11ytext::TF;

// Число без хвостовых нулей: 1.50 -> "1.5", 2.00 -> "2".
inline juce::String trimmed (double v, int decimals)
{
    auto s = juce::String (v, decimals);
    if (s.containsChar ('.'))
    {
        s = s.trimCharactersAtEnd ("0");
        if (s.endsWithChar ('.')) s = s.dropLastCharacters (1);
    }
    return s == "-0" ? juce::String ("0") : s;
}

// Частота: "61.5 Hz", "440 Hz", "1.25 kHz", "12.5 kHz".
inline juce::String hz (double f)
{
    if (f >= 1000.0) return TF ("{0} kHz", { trimmed (f / 1000.0, f >= 10000.0 ? 1 : 2) });
    if (f >= 100.0)  return TF ("{0} Hz", { juce::String (juce::roundToInt (f)) });
    return TF ("{0} Hz", { trimmed (f, 1) });
}

// Секунды как настройка: "0.5 seconds".
inline juce::String seconds (double s) { return TF ("{0} seconds", { trimmed (s, 1) }); }

// Время: "45 seconds", "1 minute 5.5 seconds", "2 minutes". Вместо «1:05»: двоеточие синтезаторы читают
// по-разному.
inline juce::String duration (double secs)
{
    secs = juce::jmax (0.0, std::round (secs * 10.0) / 10.0);
    const int minutes = (int) (secs / 60.0);
    const double rest = secs - 60.0 * minutes;
    juce::StringArray parts;
    if (minutes > 0) parts.add (TF (minutes == 1 ? "{0} minute" : "{0} minutes", { juce::String (minutes) }));
    if (rest >= 0.05 || minutes == 0)
    {
        const auto r = trimmed (rest, 1);
        parts.add (TF (r == "1" ? "{0} second" : "{0} seconds", { r }));
    }
    return parts.joinIntoString (" ");
}

// Децибелы: "-18.3 dB".
inline juce::String dB (const juce::String& number) { return TF ("{0} dB", { number }); }
inline juce::String dB (double v, int decimals = 1) { return dB (trimmed (v, decimals)); }

// Уровень: "-18.3 dB". Ниже пола графика — "below -90 dB": зрячий там тоже ничего не видит.
inline juce::String db (float v, int floorDb)
{
    if (! std::isfinite (v) || v < (float) -floorDb) return TF ("below {0} dB", { "-" + juce::String (floorDb) });
    return dB (v);
}

// Величина со знаком: "+3.2 dB", "-1 dB", "0 dB".
inline juce::String signedDb (float v)
{
    const auto t = trimmed (v, 1);
    return dB ((t != "0" && v > 0.0f ? "+" : "") + t);
}

inline juce::String percent (int v) { return TF ("{0} percent", { juce::String (v) }); }

// Текст с экрана -> текст для речи. Разделитель «·» становится запятой, тире и знак минус «−» — дефисом,
// перевод строки — точкой. Иначе NVDA говорит «точка по центру» или пропускает знак минус у числа.
inline juce::String clean (juce::String t)
{
    t = t.replace (juce::String::fromUTF8 ("\xc2\xb7"), ",")          // ·
         .replace (juce::String::fromUTF8 ("\xe2\x80\x94"), " - ")    // —
         .replace (juce::String::fromUTF8 ("\xe2\x80\x93"), " - ")    // –
         .replace (juce::String::fromUTF8 ("\xe2\x88\x92"), "-")      // −
         .replace (juce::String::fromUTF8 ("\xc2\xa0"), " ")          // неразрывный пробел
         .replace (juce::String::fromUTF8 ("\xe2\x88\x9e"), T ("infinite"))
         .replace ("\n", ". ");
    while (t.contains ("  ")) t = t.replace ("  ", " ");
    return t.replace (" ,", ",").replace (",,", ",").trim();
}

// Баланс каналов (плюс — правый громче): "centred", "1.2 dB louder on the right".
inline juce::String balance (float db)
{
    if (std::abs (db) < 0.05f) return T ("centred");
    return TF (db > 0.0f ? "{0} dB louder on the right" : "{0} dB louder on the left", { trimmed (std::abs (db), 1) });
}

// Панорама от -1 (лево) до +1 (право): "centre", "40 percent left".
inline juce::String pan (float p)
{
    const int pct = juce::roundToInt (std::abs (p) * 100.0f);
    if (pct < 5) return T ("centre");
    return TF (p < 0.0f ? "{0} percent left" : "{0} percent right", { juce::String (pct) });
}

// Корреляция: число и смысл словами (на экране это цвет и положение стрелки).
inline juce::String correlation (float c)
{
    const auto n = trimmed (c, 2);
    if (c < 0.0f)  return TF ("{0}, out of phase", { n });
    if (c > 0.9f)  return TF ("{0}, nearly mono", { n });
    return n;
}

// Название палитры обычным регистром (имена собственные, не переводятся).
inline juce::String palette (int index)
{
    static const char* const names[] = { "Ovni", "Inferno", "Viridis", "Spectrum" };
    return names[juce::jlimit (0, 3, index)];
}

// Класс ноты (0 = C) — так, как его называют музыканты этого языка: буквы, до-ре-ми или немецкие
// буквы с H и B. Диез словом: знак # синтезаторы читают как «решётка» или «номер».
inline juce::String pitchClass (int pc)
{
    static const char* const names[a11ytext::kNumLangs][12] = {
        { "C", "C sharp", "D", "D sharp", "E", "F", "F sharp", "G", "G sharp", "A", "A sharp", "B" },
        { "Do", "Do sostenido", "Re", "Re sostenido", "Mi", "Fa", "Fa sostenido", "Sol", "Sol sostenido", "La", "La sostenido", "Si" },
        { "D\xc3\xb3", "D\xc3\xb3 sustenido", "R\xc3\xa9", "R\xc3\xa9 sustenido", "Mi", "F\xc3\xa1", "F\xc3\xa1 sustenido", "Sol", "Sol sustenido", "L\xc3\xa1", "L\xc3\xa1 sustenido", "Si" },
        { "Do", "Do di\xc3\xa8se", "R\xc3\xa9", "R\xc3\xa9 di\xc3\xa8se", "Mi", "Fa", "Fa di\xc3\xa8se", "Sol", "Sol di\xc3\xa8se", "La", "La di\xc3\xa8se", "Si" },
        { "C", "Cis", "D", "Dis", "E", "F", "Fis", "G", "Gis", "A", "B", "H" },
        { "Do", "Do diesis", "Re", "Re diesis", "Mi", "Fa", "Fa diesis", "Sol", "Sol diesis", "La", "La diesis", "Si" },
        { "\xd0\xb4\xd0\xbe", "\xd0\xb4\xd0\xbe \xd0\xb4\xd0\xb8\xd0\xb5\xd0\xb7", "\xd1\x80\xd0\xb5", "\xd1\x80\xd0\xb5 \xd0\xb4\xd0\xb8\xd0\xb5\xd0\xb7",
          "\xd0\xbc\xd0\xb8", "\xd1\x84\xd0\xb0", "\xd1\x84\xd0\xb0 \xd0\xb4\xd0\xb8\xd0\xb5\xd0\xb7", "\xd1\x81\xd0\xbe\xd0\xbb\xd1\x8c",
          "\xd1\x81\xd0\xbe\xd0\xbb\xd1\x8c \xd0\xb4\xd0\xb8\xd0\xb5\xd0\xb7", "\xd0\xbb\xd1\x8f", "\xd0\xbb\xd1\x8f \xd0\xb4\xd0\xb8\xd0\xb5\xd0\xb7", "\xd1\x81\xd0\xb8" },
    };
    return juce::String::fromUTF8 (names[(int) a11ytext::language()][((pc % 12) + 12) % 12]);
}

inline juce::String key (int tonic, int mode)
{
    if (tonic < 0 || tonic > 11 || mode < 0) return {};
    return TF (mode == 1 ? "{0} minor" : "{0} major", { pitchClass (tonic) });
}

// Тональность с уверенностью: "A minor, confidence 0.82, best 64 percent of the time".
inline juce::String keyReading (int tonic, int mode, float confidence, float timeFraction)
{
    const auto k = key (tonic, mode);
    if (k.isEmpty()) return T ("not estimated yet");
    return TF ("{0}, confidence {1}, best {2} percent of the time",
               { k, trimmed (confidence, 2), juce::String (juce::roundToInt (timeFraction * 100.0f)) });
}

// Три самых сильных класса нот хроматограммы (12 значений, 0 = C): "A, E, C sharp".
inline juce::String strongestClasses (const float* chroma12)
{
    int order[12];
    for (int i = 0; i < 12; ++i) order[i] = i;
    std::sort (order, order + 12, [chroma12] (int a, int b) { return chroma12[a] > chroma12[b]; });
    if (! (chroma12[order[0]] > 1.0e-6f)) return T ("no signal");
    juce::StringArray names;
    for (int i = 0; i < 3; ++i)
        if (chroma12[order[i]] >= 0.25f * chroma12[order[0]]) names.add (pitchClass (order[i]));
    return names.joinIntoString (", ");
}

// Нота с октавой: "C sharp 3", "A 4" (научная нумерация, C4 — до первой октавы).
inline juce::String note (const NoteReadout& n)
{
    const int pc  = ((n.midi % 12) + 12) % 12;
    const int oct = (int) std::floor ((double) n.midi / 12.0) - 1;
    return pitchClass (pc) + " " + juce::String (oct);
}

// Нота с отклонением: "A 4 plus 3 cents". Ноль центов не произносим.
inline juce::String noteWithCents (const NoteReadout& n)
{
    if (n.cents == 0) return note (n);
    return TF (n.cents > 0 ? "{0} plus {1} cents" : "{0} minus {1} cents", { note (n), juce::String (std::abs (n.cents)) });
}
}
