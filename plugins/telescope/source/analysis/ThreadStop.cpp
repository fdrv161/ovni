// windows.h first, trimmed, so its min/max macros never meet JUCE's code.
#ifdef _WIN32
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif
#include "analysis/ThreadStop.h"

namespace telescope
{
bool osThreadIsGone (juce::Thread::ThreadID id) noexcept
{
   #ifdef _WIN32
    // On Windows a juce::Thread::ThreadID holds the numeric thread id.
    const auto numeric = (DWORD) (juce::pointer_sized_uint) id;
    // Id 0 with JUCE's handle still set is a thread in the last instructions of a normal exit (JUCE clears the
    // id before the handle): not gone, the next isThreadRunning() check sees it leave. Saying "gone" here made
    // stopThread (0) call TerminateThread on a handle the thread had just closed (audit M2).
    if (numeric == 0) return false;
    HANDLE h = OpenThread (SYNCHRONIZE, FALSE, numeric);
    // Only "no such thread" counts; any other failure keeps waiting, so a live thread is never given up on.
    if (h == nullptr) return GetLastError() == ERROR_INVALID_PARAMETER;
    const bool gone = WaitForSingleObject (h, 0) == WAIT_OBJECT_0;   // signalled = it has ended
    CloseHandle (h);
    return gone;
   #else
    juce::ignoreUnused (id);
    return false;
   #endif
}
}
