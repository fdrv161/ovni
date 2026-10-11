#include "ScreenReaderAnnouncer.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <oleauto.h>
 #include <UIAutomation.h>
 #if JUCE_MSVC
  #pragma comment (lib, "oleaut32.lib") // SysAllocString / SysFreeString
 #endif
#endif

namespace a11y
{
#if JUCE_WINDOWS
namespace
{
// Enum values from UIAutomationCore.h (Windows 10 1709+). Declared here
// so we do not depend on the Windows SDK version or the _WIN32_WINNT setting.
constexpr int notificationKindOther          = 4; // NotificationKind_Other
constexpr int processingImportantAll         = 0; // NotificationProcessing_ImportantAll
constexpr int processingImportantMostRecent  = 1; // NotificationProcessing_ImportantMostRecent

using RaiseNotificationFn = HRESULT (WINAPI*) (IRawElementProviderSimple*, int, int, BSTR, BSTR);
using ClientsListeningFn  = BOOL (WINAPI*)();

struct UiaFunctions
{
    UiaFunctions()
    {
        // Loaded at run time, not linked: on Windows without this function the plugin
        // should simply stay silent instead of failing to load in the DAW.
        if (auto* module = ::LoadLibraryW (L"UIAutomationCore.dll"))
        {
            raise     = reinterpret_cast<RaiseNotificationFn> (::GetProcAddress (module, "UiaRaiseNotificationEvent"));
            listening = reinterpret_cast<ClientsListeningFn>  (::GetProcAddress (module, "UiaClientsAreListening"));
        }
    }

    RaiseNotificationFn raise = nullptr;
    ClientsListeningFn listening = nullptr;
};

const UiaFunctions& uia()
{
    static const UiaFunctions instance;
    return instance;
}
} // namespace
#endif

void announce (juce::Component& source, const juce::String& text, SpeechQueue queue)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (text.isEmpty())
        return;

   #if JUCE_WINDOWS
    // We do not use juce::AccessibilityHandler::postAnnouncement: on Windows JUCE
    // speaks it with the SAPI synthesizer (the Narrator voice), and NVDA and JAWS
    // never receive the message.
    const auto& functions = uia();
    if (functions.raise == nullptr)
        return;
    if (functions.listening != nullptr && ! functions.listening())
        return; // no screen reader running: nobody to speak to

    // nullptr if the component is excluded from the accessibility tree (setAccessible(false))
    // or not on screen yet: NVDA could not tie such an event to the window.
    auto* handler = source.getAccessibilityHandler();
    if (handler == nullptr)
        return;

    // On Windows the JUCE native implementation is the UI Automation provider COM object.
    auto* unknown = reinterpret_cast<IUnknown*> (handler->getNativeImplementation());
    if (unknown == nullptr)
        return;

    IRawElementProviderSimple* provider = nullptr;
    if (FAILED (unknown->QueryInterface (__uuidof (IRawElementProviderSimple),
                                         reinterpret_cast<void**> (&provider)))
        || provider == nullptr)
        return;

    auto* message  = ::SysAllocString (text.toWideCharPointer());
    auto* activity = ::SysAllocString (L"PluginAnnouncement");

    functions.raise (provider,
                     notificationKindOther,
                     queue == SpeechQueue::replacePrevious ? processingImportantMostRecent
                                                           : processingImportantAll,
                     message,
                     activity);

    ::SysFreeString (message);
    ::SysFreeString (activity);
    provider->Release();
   #else
    juce::ignoreUnused (source);
    // On macOS JUCE passes this to VoiceOver as a standard notification.
    juce::AccessibilityHandler::postAnnouncement (text,
        queue == SpeechQueue::replacePrevious ? juce::AccessibilityHandler::AnnouncementPriority::high
                                              : juce::AccessibilityHandler::AnnouncementPriority::medium);
   #endif
}
} // namespace a11y
