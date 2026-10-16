#pragma once
#include <juce_core/juce_core.h>

namespace telescope
{
// True when the operating system no longer runs the thread with this id, although juce::Thread may still
// think it does. On Windows that happens at process exit: ExitProcess ends every other thread first and then
// runs the DLL's static destructors, and a thread ended that way never clears JUCE's handle. Elsewhere it
// always answers false (exit() does not end other threads). See ThreadStop.cpp.
bool osThreadIsGone (juce::Thread::ThreadID id) noexcept;

// Stops a juce::Thread WITHOUT ever killing a live thread.
//
// juce::Thread::stopThread (timeout) kills the thread when the timeout runs out (TerminateThread on
// Windows). A killed thread can leave the heap lock or a CriticalSection taken inside the host, and
// TerminateThread returns before the thread is really gone, so it may keep running on freed memory.
// That is the same hang OVNI Audio describes in TELESCOPE 0.3: the DAW stays in the task manager after
// it is closed, or crashes when the plugin is removed while a file is being analysed.
//
// So: ask the thread to exit, wake it, and wait for as long as it takes. The grace period only decides
// when to write a log line. One exception: if the system has already ended the thread (process exit on
// Windows), JUCE would report it running forever and the wait would never end; then stopThread (0) only
// clears JUCE's state, there is nothing left to kill. Returns true if the thread left normally in time.
inline bool stopThreadWithoutKilling (juce::Thread& t, int graceMs)
{
    if (! t.isThreadRunning()) return true;

    t.signalThreadShouldExit();
    t.notify();

    const auto start = juce::Time::getMillisecondCounter();
    bool inTime = true;
    while (t.isThreadRunning())
    {
        if (osThreadIsGone (t.getThreadId()))
        {
            t.stopThread (0);
            return false;
        }
        if (inTime && juce::Time::getMillisecondCounter() - start > (juce::uint32) graceMs)
        {
            inTime = false;
            juce::Logger::writeToLog ("TELESCOPE: thread \"" + t.getThreadName() + "\" did not exit in "
                                      + juce::String (graceMs) + " ms; waiting without a time limit");
        }
        t.waitForThreadToExit (20);
    }

    // -1 means no time limit: JUCE only kills when a timeout expires. Here it just tidies the handle.
    t.stopThread (-1);
    return inTime;
}
}
