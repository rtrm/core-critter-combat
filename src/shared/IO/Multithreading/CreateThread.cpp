#include "CreateThread.h"

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <pthread.h>
#endif

std::unique_ptr<std::thread> IO::Multithreading::CreateThreadPtr(std::string const& name, std::function<void()> entryFunction)
{
    return std::make_unique<std::thread>([name, entryFunction = std::move(entryFunction)]()
    {
       IO::Multithreading::RenameCurrentThread(name);
       entryFunction();
    });
}

std::thread IO::Multithreading::CreateThread(std::string const& name, std::function<void()> entryFunction)
{
    return std::thread([name, entryFunction = std::move(entryFunction)]()
    {
        IO::Multithreading::RenameCurrentThread(name);
        entryFunction();
    });
}

void IO::Multithreading::RenameCurrentThread(std::string const& name)
{
#if defined(WIN32)
#if defined(__MINGW32__)
    // MinGW has no working SEH __try/__except on x64 (the legacy libseh shim this
    // used to call into only emulates 32-bit x86 SEH and crashes here on x64), so
    // the old "raise a magic exception for the debugger" trick is unusable here.
    // SetThreadDescription (Win10+) is the real, non-crashing replacement; it's
    // resolved dynamically since this codebase's minimum supported Windows may be
    // older than 10. Thread naming is debugging QoL only - silently skip if absent.
    using SetThreadDescriptionFn = HRESULT(WINAPI*)(HANDLE, PCWSTR);
    static SetThreadDescriptionFn pSetThreadDescription = reinterpret_cast<SetThreadDescriptionFn>(
        reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription")));

    if (pSetThreadDescription)
    {
        std::wstring wname(name.begin(), name.end());
        pSetThreadDescription(GetCurrentThread(), wname.c_str());
    }
#else
    // Windows part taken from https://stackoverflow.com/a/23899379
    const DWORD MS_VC_EXCEPTION=0x406D1388;
#pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType;       // Must be 0x1000.
        LPCSTR szName;      // Pointer to name (in user addr space).
        DWORD dwThreadID;   // Thread ID (-1=caller thread).
        DWORD dwFlags;      // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name.c_str();
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;

    __try
    {
        RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
#endif
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    ::pthread_setname_np(pthread_self(), name.c_str());
#elif defined(__APPLE__)
    ::pthread_setname_np(name.c_str());
#else
    // It's not too serisous if we cant rename a thread
    #warning "IO::Multithreading::_renameThisThread not supported on your platform"
#endif
}
