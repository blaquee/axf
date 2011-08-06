#include "pch.h"
#include "easyhook/stdafx.h"

#include "hookah.h"
#include "hookah_nt.h"
#include "wsplugin/wspluginmanager.h"

class HookStateImpl
{
public:
    void *oldAddress;
    void *newAddress;
    HOOK_TRACE_INFO traceInfo;
};


// dont forget to initialize these variables in dllmain
HMODULE             hNtDll = NULL;
HMODULE             hKernel32 = NULL;
HMODULE             hCurrentModule = NULL;
DWORD               RhTlsIndex;
HANDLE              hEasyHookHeap = NULL;

HINSTANCE hInst = NULL;

pfnNtQueryInformationProcess _NtQueryInformationProcess;

//////////////////////////////////////////////////////////////////////////

static wchar_t *HIDDEN_DLLS[] = 
{
    HIDDEN_DLL_LIST
};

static HMODULE modules[sizeof(HIDDEN_DLLS)/sizeof(wchar_t*)];


//////////////////////////////////////////////////////////////////////////

void OpenConsole()
{
    int outHandle, errHandle, inHandle;
    FILE *outFile, *errFile, *inFile;
    AllocConsole();
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    coninfo.dwSize.Y = 9999;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    outHandle = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
    errHandle = _open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE),_O_TEXT);
    inHandle = _open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE),_O_TEXT );

    outFile = _fdopen(outHandle, "w" );
    errFile = _fdopen(errHandle, "w");
    inFile =  _fdopen(inHandle, "r");

    *stdout = *outFile;
    *stderr = *errFile;
    *stdin = *inFile;

    setvbuf( stdout, NULL, _IONBF, 0 );
    setvbuf( stderr, NULL, _IONBF, 0 );
    setvbuf( stdin, NULL, _IONBF, 0 );

    std::ios::sync_with_stdio();
}

bool IsHiddenModule( PVOID module )
{
    for (int i = 0;i < sizeof(HIDDEN_DLLS)/sizeof(wchar_t*);i++)
    {
        if(modules[i] == (HMODULE)-1L)
            modules[i] = GetModuleHandleW(HIDDEN_DLLS[i]);

        if(module == modules[i])
        {
            return true;
        }
    }

    return false;
}

#define UnlinkModuleType(moduleRecord, moduleBase, le, type) \
    for (LIST_ENTRY *l = (le)->Flink; l != (le);l = l->Flink) { \
    LDR_MODULE *ldr = CONTAINING_RECORD(l, LDR_MODULE, type); \
    if (ldr && ((ldr->BaseAddress == moduleBase) || IsHiddenModule(ldr->BaseAddress)))  { \
    l->Blink->Flink = l->Flink; \
    l->Flink->Blink = l->Blink; \
    moduleRecord = l; \
    break; \
    } \
    } 

ModuleRecord UnlinkModule( HINSTANCE moduleBase )
{
    HANDLE process = GetCurrentProcess();

    for (int i = 0;i < sizeof(modules)/sizeof(HMODULE);i++)
    {
        modules[i] = (HMODULE)-1L;
    }

    ULONG retlen=0;
    __PROCESS_BASIC_INFORMATION processInfo = {0};
    ModuleRecord moduleRecord = {0,0,0}; // save this so it can be restored later
    if(_NtQueryInformationProcess(process, ProcessBasicInformation, &processInfo, sizeof(__PROCESS_BASIC_INFORMATION), &retlen) == NT_STATUS_OK)
    {
        __PEB_LDR_DATA *ldr = processInfo.PebBaseAddress->Ldr;

        UnlinkModuleType(moduleRecord.InInitializationOrderModuleRecord, moduleBase, &ldr->InInitializationOrderModuleList, InInitializationOrderModuleList);
        UnlinkModuleType(moduleRecord.InLoadOrderModuleRecord, moduleBase, &ldr->InLoadOrderModuleList, InLoadOrderModuleList);
        UnlinkModuleType(moduleRecord.InMemoryOrderModuleRecord, moduleBase, &ldr->InMemoryOrderModuleList, InMemoryOrderModuleList);
    }

    return moduleRecord;
}

inline void LinkModuleType(LIST_ENTRY *moduleRecordLink, LIST_ENTRY &l) 
{
    LIST_ENTRY *tail = l.Blink; 
    l.Blink->Flink = moduleRecordLink; 
    l.Blink = moduleRecordLink; 
    l.Blink->Flink = l.Flink->Blink; 
    l.Blink->Blink = tail; 
}
void LinkModule(HINSTANCE moduleBase, ModuleRecord moduleRecord)
{
    HANDLE process = GetCurrentProcess();

    for (int i = 0;i < sizeof(modules)/sizeof(HMODULE);i++)
    {
        modules[i] = (HMODULE)-1L;
    }

    ULONG retlen=0;
    __PROCESS_BASIC_INFORMATION processInfo = {0};
    if(_NtQueryInformationProcess(process, ProcessBasicInformation, &processInfo, sizeof(__PROCESS_BASIC_INFORMATION), &retlen) == NT_STATUS_OK)
    {
        __PEB_LDR_DATA *ldr = processInfo.PebBaseAddress->Ldr;

        LinkModuleType(moduleRecord.InInitializationOrderModuleRecord, ldr->InInitializationOrderModuleList);
        LinkModuleType(moduleRecord.InLoadOrderModuleRecord, ldr->InLoadOrderModuleList);
        LinkModuleType(moduleRecord.InMemoryOrderModuleRecord, ldr->InMemoryOrderModuleList);
    }
}
void* PesGetProcAddress( HMODULE base, const char *name )
{
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)base;
    UINT_PTR cbase = (UINT_PTR)base;

    if(dos->e_magic == IMAGE_DOS_SIGNATURE)
    {

        IMAGE_NT_HEADERS *pe = (IMAGE_NT_HEADERS*)(cbase + dos->e_lfanew);
        UINT_PTR cpe = (UINT_PTR)pe;
        if(pe->Signature == IMAGE_NT_SIGNATURE)
        {
            UINT_PTR pentry = (UINT_PTR)pe->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
            IMAGE_EXPORT_DIRECTORY *exports = (IMAGE_EXPORT_DIRECTORY*)(cbase + pentry);

            DWORD *funcs = (DWORD*)(cbase + exports->AddressOfFunctions);
            WORD *ords = (WORD*)(cbase + exports->AddressOfNameOrdinals);
            DWORD *funcNames = (DWORD*)(cbase + exports->AddressOfNames);

            if(HIWORD(name) == 0)
            {
                for (size_t i = 0;i < exports->NumberOfNames; i++)
                {
                    if(ords[i] == LOWORD(name))
                    {
                        return (void*)(cbase+funcs[i]); 
                    }
                }
            }

            for (size_t i = 0;i < exports->NumberOfFunctions; i++)
            {
                UINT_PTR entryPoint = funcs[i];
                if(entryPoint)
                {
                    for (size_t j = 0; j < exports->NumberOfNames; j++)
                    {
                        if(ords[j] == i && strcmp(name, (char*)(funcNames[j]+cbase)) == 0)
                        {
                            return (void*)(cbase+entryPoint);
                        }
                    }

                }
            }
        }
    }

    return 0;
}

void SuspendResumeOtherThreads( bool suspendOrResume )
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    THREADENTRY32 entry = {0};
    entry.dwSize = sizeof(THREADENTRY32);

    DWORD pid = GetCurrentProcessId();
    DWORD tid = GetCurrentThreadId();

    BOOL next = Thread32First(snap, &entry);
    while(next)
    {
        HANDLE thread;
        if(entry.th32ThreadID != tid && entry.th32OwnerProcessID == pid && (thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID)))
        {
            if(suspendOrResume)
                SuspendThread(thread);
            else
                ResumeThread(thread);

            CloseHandle(thread);
        }
        next = Thread32Next(snap, &entry);           
    }
    CloseHandle(snap);
}

DWORD WINAPI StartThread( LPVOID thread )
{
    OpenConsole();


    WaitForSingleObject((HANDLE)thread, 3000);
    CloseHandle((HANDLE)thread);

    //Sleep(500);

    SuspendResumeOtherThreads(true);
    LoadNTDLLFunctions();

    PluginManager::inst();
    Init();

    UnlinkModule(hInst);
    SuspendResumeOtherThreads(false);


    return 0;
}

HANDLE DuplicateCurrentThreadHandle()
{
    HANDLE thread=0;

    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &thread, 0, FALSE, DUPLICATE_SAME_ACCESS);

    return thread;
}

BOOL EasyhookDllMain( HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
    hInst = hModule;

    DWORD tid;
    HANDLE newThread;

#ifdef _DEBUG
    int CurrentFlags;
#endif

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        {
            hCurrentModule = hModule;

            if(((hNtDll = LoadLibraryW(L"ntdll.dll")) == NULL) ||
                ((hKernel32 = LoadLibraryW(L"kernel32.dll")) == NULL))
                return FALSE;

            hEasyHookHeap = HeapCreate(0, 0, 0);   

            DbgCriticalInitialize();

            LhBarrierProcessAttach();

            LhCriticalInitialize();

            // allocate tls slot
            if((RhTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES)
                return FALSE;
            newThread = CreateThread(NULL, 0, &StartThread, DuplicateCurrentThreadHandle(), 0, &tid );
        }break;
    case DLL_THREAD_ATTACH:
        {
        }break;
    case DLL_THREAD_DETACH:
        {
            LhBarrierThreadDetach();
        }break;
    case DLL_PROCESS_DETACH:
        {
            // free tls slot
            TlsFree(RhTlsIndex);

            // remove all hooks and shutdown thread barrier...
            LhCriticalFinalize();

            LhModuleInfoFinalize();

            LhBarrierProcessDetach();

            DbgCriticalFinalize();

            //HeapDestroy(hEasyHookHeap);

            //FreeLibrary(hNtDll);
            //FreeLibrary(hKernel32);
        }break;
    }
    return TRUE;
}

HMODULE LoadNTDLLFunctions()
{
    // Load NTDLL Library and get entry address

    HMODULE hNtDll = GetModuleHandle(_T("ntdll.dll"));
    if(hNtDll == 0) return 0;

    if((_NtQueryInformationProcess = (pfnNtQueryInformationProcess)PesGetProcAddress(hNtDll, "NtQueryInformationProcess")) == 0) 
    {
        FreeLibrary(hNtDll);
        return 0;
    }

    return hNtDll;
}

//////////////////////////////////////////////////////////////////////////

HookState::HookState() : impl(new HookStateImpl)
{
    memset(&impl->traceInfo, 0, sizeof(HOOK_TRACE_INFO));
}

void * HookState::GetOldAddress() const
{
    return impl->oldAddress;
}
HookState Hookah::HookFunction( void *oldAddress, void *newAddress )
{
    if(IsHooked(oldAddress))
    {
        throw AlreadyHookedException(oldAddress);
    }


    HookState state;
    ULONG tid = 0;

    if(LhInstallHook(oldAddress, newAddress, 0, (TRACED_HOOK_HANDLE)&state.impl->traceInfo) != 0)
    {
        MessageBoxW(0, L"Failed to initialize hook, aborting now...", L"ERROR", 0);
        throw;
    }
    // activate the hook for current thread;
    if(LhSetExclusiveACL(&tid, 0, (TRACED_HOOK_HANDLE)&state.impl->traceInfo))
    {
        MessageBoxW(0, L"Failed to activate hook, aborting now...", L"ERROR", 0);
        throw;
    }

    hooks[oldAddress] = state;

    state.impl->oldAddress = oldAddress;
    state.impl->newAddress = newAddress;

    return state;
}

bool Hookah::UnhookFunction( const HookState &key )
{
    LhUninstallHook((TRACED_HOOK_HANDLE)&key.impl->traceInfo);
    LhWaitForPendingRemovals();
    hooks.erase(key.GetOldAddress());

    return true;
}

bool Hookah::IsHooked(void *oldAddress) const
{
    return hooks.find(oldAddress) != hooks.end();
}

