#pragma once
#include <juce_core/juce_core.h>

namespace telescope
{
// Stops a juce::Thread WITHOUT ever killing it.
//
// juce::Thread::stopThread (timeout) kills the thread when the timeout runs out (TerminateThread on
// Windows). A killed thread can leave the heap lock or a CriticalSection taken inside the host, and
// TerminateThread returns before the thread is really gone, so it may keep running on freed memory.
// That is the same hang OVNI Audio describes in TELESCOPE 0.3: the DAW stays in the task manager after
// it is closed, or crashes when the plugin is removed while a file is being analysed.
//
// So: ask the thread to exit, wake it, and wait for as long as it takes. The grace period only decides
// when to write a log line; it never cuts the wait short. Returns true if the thread left in time.
inline bool stopThreadWithoutKilling (juce::Thread& t, int graceMs)
{
    if (! t.isThreadRunning()) return true;

    t.signalThreadShouldExit();
    t.notify();

    const bool inTime = t.waitForThreadToExit (graceMs);
    if (! inTime)
        juce::Logger::writeToLog ("TELESCOPE: thread \"" + t.getThreadName() + "\" did not exit in "
                                  + juce::String (graceMs) + " ms; waiting without a time limit");

    // -1 means no time limit: JUCE only kills when a timeout expires. Here it just tidies the handle.
    t.stopThread (-1);
    return inTime;
}
}
