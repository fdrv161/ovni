#pragma once
#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
#include "lenses/NoteName.h"

// ========================================================================================================
// Числа для речи: коротко, без лишних нулей, единицы словами, которые синтезатор читает одинаково.
// Только ASCII: символы вроде «·», «¢», «∞» синтезаторы читают по-разному или пропускают.
// ========================================================================================================
namespace telescope::spoken
{
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
    if (f >= 1000.0) return trimmed (f / 1000.0, f >= 10000.0 ? 1 : 2) + " kHz";
    if (f >= 100.0)  return juce::String (juce::roundToInt (f)) + " Hz";
    return trimmed (f, 1) + " Hz";
}

// Время: "45 seconds", "1 minute 5.5 seconds", "2 minutes". Вместо «1:05»: двоеточие синтезаторы читают
// по-разному.
inline juce::String duration (double seconds)
{
    seconds = juce::jmax (0.0, std::round (seconds * 10.0) / 10.0);
    const int minutes = (int) (seconds / 60.0);
    const double rest = seconds - 60.0 * minutes;
    juce::String t;
    if (minutes > 0) t << minutes << (minutes == 1 ? " minute" : " minutes");
    if (rest >= 0.05 || minutes == 0)
    {
        const auto r = trimmed (rest, 1);
        t << (t.isEmpty() ? "" : " ") << r << (r == "1" ? " second" : " seconds");
    }
    return t;
}

// Уровень: "-18.3 dB". Ниже пола графика — "below -90 dB": зрячий там тоже ничего не видит.
inline juce::String db (float v, int floorDb)
{
    if (! std::isfinite (v) || v < (float) -floorDb) return "below -" + juce::String (floorDb) + " dB";
    return trimmed (v, 1) + " dB";
}

// Величина со знаком: "+3.2 dB", "-1 dB", "0 dB".
inline juce::String signedDb (float v)
{
    const auto t = trimmed (v, 1);
    return (t != "0" && v > 0.0f ? "+" : "") + t + " dB";
}

// Текст с экрана -> текст для речи. Разделитель «·» становится запятой, тире и знак минус «−» — дефисом,
// перевод строки — точкой. Иначе NVDA говорит «точка по центру» или пропускает знак минус у числа.
inline juce::String clean (juce::String t)
{
    t = t.replace (juce::String::fromUTF8 ("\xc2\xb7"), ",")          // ·
         .replace (juce::String::fromUTF8 ("\xe2\x80\x94"), " - ")    // —
         .replace (juce::String::fromUTF8 ("\xe2\x80\x93"), " - ")    // –
         .replace (juce::String::fromUTF8 ("\xe2\x88\x92"), "-")      // −
         .replace (juce::String::fromUTF8 ("\xc2\xa0"), " ")          // неразрывный пробел
         .replace (juce::String::fromUTF8 ("\xe2\x88\x9e"), "infinite")
         .replace ("\n", ". ");
    while (t.contains ("  ")) t = t.replace ("  ", " ");
    return t.replace (" ,", ",").replace (",,", ",").trim();
}

// Баланс каналов (плюс — правый громче): "centred", "1.2 dB louder on the right".
inline juce::String balance (float db)
{
    if (std::abs (db) < 0.05f) return "centred";
    return trimmed (std::abs (db), 1) + " dB louder on the " + (db > 0.0f ? "right" : "left");
}

// Панорама от -1 (лево) до +1 (право): "centre", "40 percent left".
inline juce::String pan (float p)
{
    const int pct = juce::roundToInt (std::abs (p) * 100.0f);
    if (pct < 5) return "centre";
    return juce::String (pct) + " percent " + (p < 0.0f ? "left" : "right");
}

// Корреляция: число и смысл словами (на экране это цвет и положение стрелки).
inline juce::String correlation (float c)
{
    const auto n = trimmed (c, 2);
    if (c < 0.0f)  return n + ", out of phase";
    if (c > 0.9f)  return n + ", nearly mono";
    return n;
}

// Название палитры обычным регистром.
inline juce::String palette (int index)
{
    static const char* const names[] = { "Ovni", "Inferno", "Viridis", "Spectrum" };
    return names[juce::jlimit (0, 3, index)];
}

// Класс ноты (0 = C) и тональность (mode 1 — минор): "F sharp minor".
inline juce::String pitchClass (int pc)
{
    static const char* const names[12] = { "C", "C sharp", "D", "D sharp", "E", "F", "F sharp", "G",
                                           "G sharp", "A", "A sharp", "B" };
    return names[((pc % 12) + 12) % 12];
}

inline juce::String key (int tonic, int mode)
{
    if (tonic < 0 || tonic > 11 || mode < 0) return {};
    return pitchClass (tonic) + (mode == 1 ? " minor" : " major");
}

// Тональность с уверенностью: "A minor, confidence 0.82, best 64 percent of the time".
inline juce::String keyReading (int tonic, int mode, float confidence, float timeFraction)
{
    const auto k = key (tonic, mode);
    if (k.isEmpty()) return "not estimated yet";
    return k + ", confidence " + trimmed (confidence, 2) + ", best " + juce::String (juce::roundToInt (timeFraction * 100.0f))
         + " percent of the time";
}

// Три самых сильных класса нот хроматограммы (12 значений, 0 = C): "A, E, C sharp".
inline juce::String strongestClasses (const float* chroma12)
{
    int order[12];
    for (int i = 0; i < 12; ++i) order[i] = i;
    std::sort (order, order + 12, [chroma12] (int a, int b) { return chroma12[a] > chroma12[b]; });
    if (! (chroma12[order[0]] > 1.0e-6f)) return "no signal";
    juce::StringArray names;
    for (int i = 0; i < 3; ++i)
        if (chroma12[order[i]] >= 0.25f * chroma12[order[0]]) names.add (pitchClass (order[i]));
    return names.joinIntoString (", ");
}

// Нота: "C sharp 3", "A 4". Знак # синтезаторы читают как «решётка» или «номер», поэтому словом.
inline juce::String note (const NoteReadout& n)
{
    auto name = n.name;
    const int digitPos = name.indexOfAnyOf ("-0123456789");
    auto letter = digitPos > 0 ? name.substring (0, digitPos) : name;
    const auto octave = digitPos > 0 ? name.substring (digitPos) : juce::String();
    letter = letter.replace ("#", " sharp");
    return octave.isEmpty() ? letter : letter + " " + octave;
}

// Нота с отклонением: "A 4 plus 3 cents". Ноль центов не произносим.
inline juce::String noteWithCents (const NoteReadout& n)
{
    if (n.cents == 0) return note (n);
    return note (n) + (n.cents > 0 ? " plus " : " minus ") + juce::String (std::abs (n.cents)) + " cents";
}
}
