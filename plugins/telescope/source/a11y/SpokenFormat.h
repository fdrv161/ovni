#pragma once
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
