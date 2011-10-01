#ifndef hookah_h__
#define hookah_h__

#include "singleton.h"
extern "C"
{
    // dont forget to initialize these variables in dllmain
    extern HMODULE             hNtDll;
    extern HMODULE             hKernel32;
    extern HMODULE             hCurrentModule;
    extern DWORD               RhTlsIndex;
    extern HANDLE              hEasyHookHeap;

    extern HINSTANCE hInst;
};

struct ModuleRecord
{
    LIST_ENTRY *InInitializationOrderModuleRecord;
    LIST_ENTRY *InLoadOrderModuleRecord;
    LIST_ENTRY *InMemoryOrderModuleRecord;
};

#define HIDDEN_DLL_LIST L"PYTHON31.DLL", \
                        L"_CTYPES.PYD", \
                        L"QTCORE.PYD", \
                        L"QTCORE4.DLL", \
                        L"QTGUI.PYD", \
                        L"QTGUI4.DLL", \
                        L"SIP.PYD"  

void Init(); // user implmented

BOOL EasyhookDllMain( HINSTANCE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    );

//////////////////////////////////////////////////////////////////////////
class AlreadyHookedException : public std::exception
{
    void *addr;
public:
    AlreadyHookedException(void *addr) : std::exception("Already hooked"){}
    void *GetAddress() const
    {
        return addr;
    }
};

class HookStateImpl;

class HookState
{
    friend class Hookah;

    std::shared_ptr<HookStateImpl> impl;

public:
    HookState();

    void *GetOldAddress() const;
};

class Hookah : public Singleton<Hookah>
{
    friend class Singleton<Hookah>;

    std::map<void*, HookState> hooks; // function ptr -> HookState mapping

public:
    HookState HookFunction(void *oldAddress, void *newAddress);
    bool UnhookFunction(const HookState &key);
    bool IsHooked(void *oldAddress) const;
};

//////////////////////////////////////////////////////////////////////////

bool IsHiddenModule(PVOID module);

ModuleRecord UnlinkModule(HINSTANCE moduleBase);
void LinkModule(HINSTANCE moduleBase, ModuleRecord moduleRecord);

void* PesGetProcAddress(HMODULE base, const char *name);

//////////////////////////////////////////////////////////////////////////

DWORD WINAPI StartThread(LPVOID thread);

HANDLE DuplicateCurrentThreadHandle();

void SuspendResumeOtherThreads(bool suspendOrResume);


#endif // hookah_h__

