#pragma once
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "a11y/ScreenReaderAnnouncer.h"

// ========================================================================================================
// Доступные элементы TELESCOPE для скринридера и клавиатуры.
//
// Весь интерфейс плагина нарисован картинкой, поэтому NVDA там не видит ни одного элемента. Эти компоненты
// лежат ПОВЕРХ нарисованного (мышь проходит сквозь них к старому интерфейсу) и дают скринридеру настоящие
// элементы с фокусом, именами и клавишами. Рисуют они только рамку фокуса.
// ========================================================================================================
namespace telescope::a11yui
{
// Шаги изменения значения: обычный, крупный и «до предела» (Alt+Home/End).
inline constexpr int kStep    = 1;
inline constexpr int kCoarse  = 10;
inline constexpr int kExtreme = 1000;

// Откуда сетка берёт строки. Группы — это «страницы» (показания, полосы, настройки), строки — «Имя, значение».
struct GridSource
{
    virtual ~GridSource() = default;
    virtual int          groupCount() const = 0;
    virtual juce::String groupName (int group) const = 0;
    virtual int          rowCount (int group) const = 0;
    virtual juce::String rowName (int group, int row) const = 0;
    virtual juce::String rowValue (int group, int row) const = 0;
    virtual bool         isAdjustable (int, int) const { return false; }
    // step: ±kStep, ±kCoarse или ±kExtreme. true — значение изменилось (на краю диапазона — false, тишина).
    virtual bool         adjust (int, int, int) { return false; }
    // Строка-кнопка (Reset, Load…): Enter выполняет действие. Возвращает фразу о результате (может быть пустой,
    // например когда открывается диалог — его озвучит сам скринридер).
    virtual bool         isActivatable (int, int) const { return false; }
    virtual juce::String activate (int, int) { return {}; }
};

// Общая часть: живое имя для скринридера, рамка фокуса, Tab и F1.
class FocusOverlay : public juce::Component
{
public:
    std::function<void (bool forward)> onTab;
    std::function<void()>              onEscape;
    juce::String                       helpText;   // F1

    FocusOverlay()
    {
        setWantsKeyboardFocus (true);
        // Мышь идёт сквозь элемент к нарисованному интерфейсу: зрячий пользователь работает как раньше.
        setInterceptsMouseClicks (false, false);
    }

    // Текст, который скринридер читает как имя элемента (NVDA+Стрелка вверх). Вычисляется при каждом
    // запросе, поэтому всегда свежий и не требует событий.
    virtual juce::String liveTitle() const = 0;
    // Описание читается после имени и роли при входе фокуса: место для контекста («Spectrum, Readings»).
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
    // Общие клавиши. true — клавиша обработана здесь.
    bool handleCommonKey (const juce::KeyPress& key)
    {
        const auto mods = key.getModifiers();
        const int  code = key.getKeyCode();
        const bool noCtrlAlt = ! mods.isCtrlDown() && ! mods.isAltDown() && ! mods.isCommandDown();

        if (code == juce::KeyPress::tabKey && noCtrlAlt && onTab) { onTab (! mods.isShiftDown()); return true; }
        if (code == juce::KeyPress::F1Key && ! mods.isAnyModifierKeyDown() && helpText.isNotEmpty())
        {
            say (helpText, a11y::SpeechQueue::keepAll);
            return true;
        }
        if (code == juce::KeyPress::escapeKey && ! mods.isAnyModifierKeyDown() && onEscape) { onEscape(); return true; }
        return false;
    }

    // Что делать с клавишей, которую элемент не знает. Комбинации с Ctrl/Alt и F-клавиши отдаём DAW
    // (Ctrl+S сохраняет проект, Alt+F4 закрывает окно). Пробел — тоже DAW: старт и стоп транспорта нужен,
    // пока слушаешь анализ. Остальные «голые» клавиши поглощаем: иначе стрелки, Delete или буквы
    // запустили бы действия REAPER.
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
            juce::String getTitle() const override       { return owner.liveTitle(); }
            juce::String getDescription() const override { return owner.liveDescription(); }
        private:
            FocusOverlay& owner;
        };
        return std::make_unique<LiveTitleHandler> (*this, role, std::move (actions));
    }
};

// ========================================================================================================
// Сетка «группы × строки» в ОДНОМ фокусируемом элементе (модель из LSampler-24):
//   Стрелки вверх/вниз — строки группы; влево/вправо — соседняя группа; Home/End — край группы;
//   Ctrl+Home/End — начало/конец всей сетки; Page Up/Down — на 10 строк;
//   Alt+Стрелка вверх/вниз — изменить значение на шаг; Alt+Page Up/Down — крупный шаг;
//   Alt+Home/End — минимум/максимум; Enter или F5 — перечитать строку со свежим значением;
//   буква или цифра — следующая строка на этот знак (Shift — назад); F1 — справка; Esc — назад.
// Края тихие, без перехода по кругу.
// ========================================================================================================
class A11yGrid : public FocusOverlay
{
public:
    std::function<void (int group, int row)> onCursorChanged;   // курсор сдвинул пользователь
    std::function<void()>                    onEnter;           // если не задан — Enter перечитывает строку
    juce::String emptyText { "Empty" };

    explicit A11yGrid (juce::AccessibilityRole r = juce::AccessibilityRole::list) : role (r) {}

    void setSource (GridSource* s) { source = s; clampCursor(); }

    int group() const noexcept { return grp; }
    int row()   const noexcept { return rw; }

    juce::String rowText (int g, int r) const
    {
        if (source == nullptr) return emptyText;
        const auto name  = source->rowName (g, r);
        const auto value = source->rowValue (g, r);
        return value.isEmpty() ? name : name + ", " + value;
    }

    juce::String currentText() const
    {
        if (source == nullptr || source->groupCount() <= 0 || source->rowCount (grp) <= 0) return emptyText;
        return rowText (grp, rw);
    }

    juce::String liveTitle() const override { return currentText(); }

    std::function<juce::String()> contextName;   // например, имя линзы: «Spectrum»

    juce::String liveDescription() const override
    {
        juce::StringArray parts;
        if (contextName) parts.add (contextName());
        if (source != nullptr && source->groupCount() > 1) parts.add (source->groupName (grp));
        parts.removeEmptyStrings();
        return parts.joinIntoString (", ");
    }

    // Программная установка курсора (без речи, если speak = false).
    void setCursor (int g, int r, bool speak, bool withGroup = false)
    {
        grp = g; rw = r;
        clampCursor();
        repaint();
        if (speak) speakCurrent (withGroup);
    }

    // Источник поменялся (другая линза): удержать курсор в границах, ничего не говорить.
    void refresh() { clampCursor(); repaint(); }

    void speakCurrent (bool withGroup)
    {
        if (source == nullptr) return;
        say (withGroup && source->groupCount() > 1 ? source->groupName (grp) + ". " + currentText()
                                                   : currentText());
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (handleCommonKey (key)) return true;
        if (source == nullptr) return swallowUnknown (key);

        const auto mods  = key.getModifiers();
        const int  code  = key.getKeyCode();
        const bool plain = ! mods.isAnyModifierKeyDown();
        const bool ctrl  = mods.isCtrlDown() && ! mods.isAltDown() && ! mods.isShiftDown();
        const bool alt   = mods.isAltDown() && ! mods.isCtrlDown() && ! mods.isShiftDown();
        const int  rows  = source->rowCount (grp);
        const int  groups = source->groupCount();

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

        if (alt)
        {
            int step = 0;
            if (code == juce::KeyPress::upKey)            step = kStep;
            else if (code == juce::KeyPress::downKey)     step = -kStep;
            else if (code == juce::KeyPress::pageUpKey)   step = kCoarse;
            else if (code == juce::KeyPress::pageDownKey) step = -kCoarse;
            else if (code == juce::KeyPress::endKey)      step = kExtreme;
            else if (code == juce::KeyPress::homeKey)     step = -kExtreme;

            if (step != 0)
            {
                if (! source->isAdjustable (grp, rw)) { say ("Read only"); return true; }
                // Изменилось — говорим только новое значение. На краю — тишина.
                if (source->adjust (grp, rw, step)) say (source->rowValue (grp, rw));
                return true;
            }
        }

        // Поиск по первому знаку имени. getTextCharacter, а не код клавиши: иначе стрелки и Page Up
        // попадут в «печатный» диапазон кодов (известная ловушка JUCE).
        if (! mods.isCtrlDown() && ! mods.isAltDown() && ! mods.isCommandDown())
        {
            auto typed = key.getTextCharacter();
            if (mods.isShiftDown() && ((code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9')))
                typed = (juce::juce_wchar) code;   // Shift+цифра даёт «!» — берём саму цифру
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

private:
    void clampCursor()
    {
        const int groups = source != nullptr ? source->groupCount() : 0;
        grp = groups > 0 ? juce::jlimit (0, groups - 1, grp) : 0;
        const int rows = source != nullptr && groups > 0 ? source->rowCount (grp) : 0;
        rw = rows > 0 ? juce::jlimit (0, rows - 1, rw) : 0;
    }

    void moveTo (int g, int r, bool withGroup)
    {
        const int oldG = grp, oldR = rw;
        grp = g; rw = r;
        clampCursor();
        if (grp == oldG && rw == oldR) return;   // край — тишина
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
        // Нет совпадений — тишина, как на краях.
    }

    juce::AccessibilityRole role;
    GridSource* source = nullptr;
    int grp = 0, rw = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (A11yGrid)
};

// ========================================================================================================
// Кнопка поверх нарисованной строки. Enter или пробел — нажать. Имя живое: «Language, English».
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
        // Действие press: NVDA умеет нажимать кнопку и своей командой, без клавиш плагина.
        return makeHandler (juce::AccessibilityRole::button,
                            juce::AccessibilityActions().addAction (juce::AccessibilityActionType::press,
                                                                    [this] { press(); }));
    }
};
}
