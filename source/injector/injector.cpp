#include <Windows.h>
#include <shlwapi.h>
#include <string.h>
#include <string>
#include <sstream>
#include <fstream>
#include <ObjBase.h>
#include <ObjIdl.h>
#include <Shobjidl.h>
#include <Shlwapi.h>
#include <TlHelp32.h>

#include "regkey.h"

#define REGISTRY_DIR L"Software\\AXF"

#define DLL_NAME L"\\AnExtensionFramework.dll"

// extra paths in the current working dir to append to the PATH env variable
//static const wchar_t *EXTRA_PATHS[] =  { L"python" };

//#define SPLASH_SCREEN
//#include "Splash.h"

static void InjectDLL_RemoteThread(HANDLE hProc,const wchar_t *dll);

//global vars
static wchar_t currentDir[2000];
static unsigned char bpInstruction = 0xcc; // store the original instruction here

using namespace std;


#ifndef NT_STATUS_OK
#define NT_STATUS_OK 0
#endif

typedef
    VOID
    (NTAPI *__PPS_POST_PROCESS_INIT_ROUTINE) (
    VOID
    );

typedef struct tag_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} __UNICODE_STRING;

typedef struct _LDR_MODULE {
    LIST_ENTRY              InLoadOrderModuleList;
    LIST_ENTRY              InMemoryOrderModuleList;
    LIST_ENTRY              InInitializationOrderModuleList;
    PVOID                   BaseAddress;
    PVOID                   EntryPoint;
    ULONG                   SizeOfImage;
    __UNICODE_STRING          FullDllName;
    __UNICODE_STRING          BaseDllName;
    ULONG                   Flags;
    SHORT                   LoadCount;
    SHORT                   TlsIndex;
    LIST_ENTRY              HashTableEntry;
    ULONG                   TimeDateStamp;

} LDR_MODULE, *PLDR_MODULE;


typedef struct tag_PEB_LDR_DATA {
    ULONG                   Length;
    BOOLEAN                 Initialized;
    PVOID                   SsHandle;
    LIST_ENTRY              InLoadOrderModuleList;
    LIST_ENTRY              InMemoryOrderModuleList;
    LIST_ENTRY              InInitializationOrderModuleList;
} __PEB_LDR_DATA, *__PPEB_LDR_DATA;

typedef struct tag_RTL_USER_PROCESS_PARAMETERS {
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    __UNICODE_STRING ImagePathName;
    __UNICODE_STRING CommandLine;
} __RTL_USER_PROCESS_PARAMETERS, *__PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB {
    BYTE InheritedAddressSpace;
    BYTE ReadImageFileExecOptions;
    BYTE BeingDebugged;
    BYTE Spare;
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    __PPEB_LDR_DATA Ldr;
    __PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    BYTE Reserved4[104];
    PVOID Reserved5[52];
    __PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    BYTE Reserved6[128];
    PVOID Reserved7[1];
    ULONG SessionId;
} __PEB, *__PPEB;

typedef struct tag_PROCESS_BASIC_INFORMATION {
    PVOID ExitStatus; //cast to NTSTATUS
    __PPEB PebBaseAddress;
    PVOID AffinityMask; 
    PVOID BasePriority; 
    ULONG_PTR UniqueProcessId;
    PVOID InheritedFromUniqueProcessId;
} __PROCESS_BASIC_INFORMATION;

typedef __PROCESS_BASIC_INFORMATION *__PPROCESS_BASIC_INFORMATION;

// found in ntddk.h
typedef enum tag__PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessWow64Information = 26
} __PROCESSINFOCLASS;


typedef enum tag_SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3,
    SystemProcessInformation = 5,
    SystemProcessorPerformanceInformation = 8,
    SystemInterruptInformation = 23,
    SystemExceptionInformation = 33,
    SystemRegistryQuotaInformation = 37,
    SystemLookasideInformation = 45
} __SYSTEM_INFORMATION_CLASS;

typedef struct _CLIENT_ID{ 
    HANDLE UniqueProcess; 
    HANDLE UniqueThread; 
} CLIENT_ID, *PCLIENT_ID;

typedef struct tag_SYSTEM_THREAD {
    LARGE_INTEGER           KernelTime;
    LARGE_INTEGER           UserTime;
    LARGE_INTEGER           CreateTime;
    ULONG                   WaitTime;
    PVOID                   StartAddress;
    CLIENT_ID               ClientId;
    LONG                    Priority;
    LONG                    BasePriority;
    ULONG                   ContextSwitchCount;
    ULONG                   State;
    LONG                    WaitReason;

} __SYSTEM_THREAD, *__PSYSTEM_THREAD;

typedef struct tag_SYSTEM_PROCESS_INFORMATION {
    ULONG                   NextEntryOffset; // when this entry is 0, there are no more entries left
    ULONG                   NumberOfThreads;
    LARGE_INTEGER           Reserved[3];
    LARGE_INTEGER           CreateTime;
    LARGE_INTEGER           UserTime;
    LARGE_INTEGER           KernelTime;
    __UNICODE_STRING          ImageName;
    LONG                   BasePriority;
    HANDLE                  ProcessId;
    HANDLE                  InheritedFromProcessId;
    ULONG                   HandleCount;
    ULONG              SessionId;
    ULONG              PageDirectoryBase;
    LONG                PeakVirtualSize;
    LONG                VirtualSize;
    ULONG              PageFaultCount;
    LONG                PeakWorkingSetSize;
    LONG                WorkingSetSize;
    LONG                QuotaPeakPagedPoolUsage;
    LONG                QuotaPagedPoolUsage;
    LONG                QuotaPeakNonPagedPoolUsage;
    LONG                QuotaNonPagedPoolUsage;
    LONG                PagefileUsage;
    LONG                PeakPagefileUsage;
    LONG                PrivatePageCount;
    LARGE_INTEGER              ReadOperationCount;
    LARGE_INTEGER              WriteOperationCount;
    LARGE_INTEGER              OtherOperationCount;
    LARGE_INTEGER              ReadTransferCount;
    LARGE_INTEGER              WriteTransferCount;
    LARGE_INTEGER              OtherTransferCount;
    
    //__SYSTEM_THREAD           Threads[]; //not a pointer, but an array

} __SYSTEM_PROCESS_INFORMATION, *__PSYSTEM_PROCESS_INFORMATION;


typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    __UNICODE_STRING *ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} __OBJECT_ATTRIBUTES, *__POBJECT_ATTRIBUTES;


typedef NTSTATUS (NTAPI *pNtQuerySystemInformation)(
    __in       __SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __inout    PVOID SystemInformation,
    __in       ULONG SystemInformationLength,
    __out_opt  PULONG ReturnLength
    );

typedef NTSTATUS (NTAPI *pNtQueryInformationProcess)(
    IN  HANDLE ProcessHandle,
    IN  __PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN  ULONG ProcessInformationLength,
    OUT PULONG ReturnLength    OPTIONAL
    );

typedef NTSTATUS (NTAPI *pNtOpenThread)(PHANDLE phThread, ACCESS_MASK AccessMask, __POBJECT_ATTRIBUTES pObjectAttributes, PCLIENT_ID pClientId); 
typedef NTSTATUS (NTAPI *pNtSuspendThread)(HANDLE ThreadHandle, PDWORD PreviousSuspendCount);
typedef NTSTATUS (NTAPI *pNtResumeThread)(HANDLE ThreadHandle, PDWORD PreviousSuspendCount);
typedef NTSTATUS (NTAPI *pNtClose)(HANDLE ObjectHandle);


static pNtQueryInformationProcess _NtQueryInformationProcess;
static pNtQuerySystemInformation _NtQuerySystemInformation;
static pNtOpenThread _NtOpenThread;
static pNtSuspendThread _NtSuspendThread;
static pNtResumeThread _NtResumeThread;
static pNtClose _NtClose;

static HMODULE LoadNTDLLFunctions()
{
    // Load NTDLL Library and get entry address

    HMODULE hNtDll = LoadLibrary(_T("ntdll.dll"));
    if(hNtDll == 0) return 0;

    if((_NtQueryInformationProcess = (pNtQueryInformationProcess)GetProcAddress(hNtDll, "NtQueryInformationProcess")) == 0) 
    {
        FreeLibrary(hNtDll);
        return 0;
    }

    if((_NtQuerySystemInformation = (pNtQuerySystemInformation)GetProcAddress(hNtDll, "NtQuerySystemInformation")) == 0)
    {
        FreeLibrary(hNtDll);
        return 0;
    }

    if((_NtOpenThread = (pNtOpenThread)GetProcAddress(hNtDll, "NtOpenThread")) == 0)
    {
        FreeLibrary(hNtDll);
        return 0;
    }


    if((_NtSuspendThread = (pNtSuspendThread)GetProcAddress(hNtDll, "NtSuspendThread")) == 0)
    {
        FreeLibrary(hNtDll);
        return 0;
    }

    if((_NtResumeThread = (pNtResumeThread)GetProcAddress(hNtDll, "NtResumeThread")) == 0)
    {
        FreeLibrary(hNtDll);
        return 0;
    }

    if((_NtClose = (pNtClose)GetProcAddress(hNtDll, "NtClose")) == 0)
    {
        FreeLibrary(hNtDll);
        return 0;
    }

    return hNtDll;
}

struct StaticInitializer
{
    StaticInitializer()
    {
        if(LoadNTDLLFunctions() == 0)
        {
            MessageBoxW(0, L"Failed to load ntdll, are you running winxp or better?", L"Error", 0);
            return;
        }
    }
}StaticInitializer;


static void BreakpointEntry(HANDLE process, UINT_PTR entry)
{
    SIZE_T ret;
    unsigned char bp = 0xCC;
    DWORD oldProt;
    VirtualProtectEx(process, (LPVOID)entry, sizeof(char), PAGE_EXECUTE_READWRITE, &oldProt);
    ReadProcessMemory(process, (LPVOID)entry, &bpInstruction, sizeof(char), &ret);
    WriteProcessMemory(process, (LPVOID)entry, &bp, sizeof(char), &ret);
    VirtualProtectEx(process, (LPVOID)entry, sizeof(char), oldProt, &oldProt);
}

static UINT_PTR GetEntryPointOfImage(HANDLE process)
{
    ULONG retlen=0;
    __PROCESS_BASIC_INFORMATION processInfo = {0};
    if(_NtQueryInformationProcess(process, ProcessBasicInformation, &processInfo, sizeof(__PROCESS_BASIC_INFORMATION), &retlen) == NT_STATUS_OK)
    {
        __PEB peb = {0};
        SIZE_T ret;

        ReadProcessMemory(process, processInfo.PebBaseAddress, &peb, sizeof(__PEB), &ret);

        IMAGE_DOS_HEADER dosHdr;
        ReadProcessMemory(process, peb.ImageBaseAddress, &dosHdr, sizeof(IMAGE_DOS_HEADER), &ret);
        if(dosHdr.e_magic == IMAGE_DOS_SIGNATURE)
        {
            IMAGE_NT_HEADERS nt;
            ReadProcessMemory(process, LPCVOID(UINT_PTR(peb.ImageBaseAddress)+UINT_PTR(dosHdr.e_lfanew)), &nt, sizeof(IMAGE_NT_HEADERS), &ret);
            if(nt.Signature == IMAGE_NT_SIGNATURE)
            {
                return UINT_PTR(peb.ImageBaseAddress) + UINT_PTR(nt.OptionalHeader.AddressOfEntryPoint);
            }
        }

    }

    return 0;
}

__SYSTEM_THREAD *GetSystemProcessThreadEntry(__SYSTEM_PROCESS_INFORMATION **spi)
{
    return (__SYSTEM_THREAD*)(UINT_PTR(*spi) + UINT_PTR(sizeof(__SYSTEM_PROCESS_INFORMATION)));
}


static void SuspendResumeProcess(DWORD processID, bool suspendOrResume) //true for suspend, otherwise resume
{

    __SYSTEM_PROCESS_INFORMATION *pi, *piOrig;
    ULONG size;
    _NtQuerySystemInformation(SystemProcessInformation, 0, 0, &size);
    pi = piOrig = (__SYSTEM_PROCESS_INFORMATION*)malloc(size);
    _NtQuerySystemInformation(SystemProcessInformation, pi, size, &size);

    while(pi->NextEntryOffset != 0)
    {
        
        __SYSTEM_THREAD *thread = GetSystemProcessThreadEntry(&pi);

        
        if(pi->ProcessId == (HANDLE)processID)
        {
            
            for(int i=0;i < pi->NumberOfThreads;i++,thread += sizeof(__SYSTEM_THREAD))
            {
                __OBJECT_ATTRIBUTES obj = {0};
                obj.Length = sizeof(__OBJECT_ATTRIBUTES);
                HANDLE hTh = 0;
                if(_NtOpenThread(&hTh, THREAD_SUSPEND_RESUME, &obj, &thread->ClientId) == NT_STATUS_OK)
                {
                    DWORD suspendCount;
                    if(suspendOrResume)
                        _NtSuspendThread(hTh, &suspendCount);
                    else
                        _NtResumeThread(hTh, &suspendCount);
                    _NtClose(hTh);
                }

            }

            
        }

        pi = (__SYSTEM_PROCESS_INFORMATION*)(UINT_PTR(pi) + UINT_PTR(pi->NextEntryOffset));
    }

    free(piOrig);

}

static void SuspendProcess(DWORD processID)
{
    SuspendResumeProcess(processID, true);
}

static void ResumeProcess(DWORD processID)
{
    SuspendResumeProcess(processID, false);
}


static void InjectOnEntryPoint(DWORD processID, HANDLE process, UINT_PTR entry, const wchar_t *dllPath)
{
    DWORD dwContinueStatus = DBG_CONTINUE; // exception continuation 

    DEBUG_EVENT debugEv;
    SIZE_T ret;

    DWORD oldProt;

    while(true) 
    { 
        WaitForDebugEvent(&debugEv, INFINITE); 
        switch(debugEv.dwDebugEventCode)
        {
        case EXCEPTION_DEBUG_EVENT:
            switch(debugEv.u.Exception.ExceptionRecord.ExceptionCode)
            {
            case EXCEPTION_BREAKPOINT:
                VirtualProtectEx(process, (LPVOID)entry, sizeof(char), PAGE_EXECUTE_READWRITE, &oldProt);
                WriteProcessMemory(process, (LPVOID)entry, &bpInstruction, sizeof(char), &ret);
                VirtualProtectEx(process, (LPVOID)entry, sizeof(char), oldProt, &oldProt);
                InjectDLL_RemoteThread(process, dllPath);
                ContinueDebugEvent(debugEv.dwProcessId, debugEv.dwThreadId, dwContinueStatus);
                return;
            }
            break;
        }

        ContinueDebugEvent(debugEv.dwProcessId, debugEv.dwThreadId, dwContinueStatus);
    }
}

static void KeepDebuggingUntilExit()
{
    DWORD dwContinueStatus = DBG_CONTINUE; // exception continuation 
    DEBUG_EVENT debugEv;

    while(true) 
    { 
        WaitForDebugEvent(&debugEv, INFINITE); 
        switch(debugEv.dwDebugEventCode)
        {
            case EXCEPTION_DEBUG_EVENT:
                ContinueDebugEvent(debugEv.dwProcessId, debugEv.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
                break;

        case EXIT_PROCESS_DEBUG_EVENT:
            return;
        }

        ContinueDebugEvent(debugEv.dwProcessId, debugEv.dwThreadId, dwContinueStatus);
    }
}

static bool IsWindows2000()
{
    OSVERSIONINFO ver;
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&ver);

    return (ver.dwMajorVersion == 5 && ver.dwMinorVersion == 0);
}

static void InjectDLL_RemoteThread(HANDLE hProc,const wchar_t *dll) 
{
    SIZE_T written=0;
    size_t pathLen = (wcslen(dll)+2)*sizeof(wchar_t);

    LPVOID remoteDll = (char*)VirtualAllocEx(hProc,0,pathLen*2,MEM_COMMIT,PAGE_READWRITE);

    if(remoteDll == 0)
    {
        MessageBoxW(0, L"Could not allocated memory", L"Error", 0);
        return;
    }

    WriteProcessMemory(hProc,remoteDll,dll,pathLen,&written);
    if (written != pathLen)
    {
        MessageBoxW(0,L"Failed to inject",L"error",MB_ICONEXCLAMATION|MB_OK);
        return;
    }

    HMODULE kernel32 = GetModuleHandle(L"kernel32.dll");

    if(kernel32 == 0)
    {
        MessageBoxW(0, L"Failed to open kernel32", L"Error", 0);
        return;
    }

    UINT_PTR remoteLoadLib = (UINT_PTR)GetProcAddress(kernel32, "LoadLibraryW");


    DWORD th;
    HANDLE hTh = CreateRemoteThread(hProc,0,0,(LPTHREAD_START_ROUTINE)remoteLoadLib,remoteDll,0,&th);

    if(hTh == 0)
    {
        MessageBoxW(0, L"Failed to create thread", L"Error", 0);
        return;
    }
}

inline string GetDirFromString(const string &str)
{
    typedef string::size_type SizeType;

    SizeType pos = str.rfind("\\");

    return pos != string::npos ? str.substr( 0,  pos+1 ) : "";
}

inline basic_string<wchar_t> GetDirFromString(const basic_string<wchar_t> &str)
{
    typedef basic_string<wchar_t>::size_type SizeType;

    SizeType pos = str.rfind(L"\\");

    return pos != basic_string<wchar_t>::npos ? str.substr( 0,  pos+1 ) : L"";
}

static bool openFile(PROCESS_INFORMATION &pi)
{

    OPENFILENAMEW ofn;
    memset(&ofn,0,sizeof(OPENFILENAME));
    ofn.hInstance = GetModuleHandle(0);
    ofn.lpstrTitle = L"Select a target";
    ofn.lStructSize = sizeof(OPENFILENAME);          //ofn.
    ofn.lpstrFilter = L"App Files (*.exe, *.bat, *.lnk)\0*.exe;*.bat;*.lnk\0" L"All Files (*.*)\0*.*\0" L"\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS;
    ofn.lpstrFile = new wchar_t[2000];
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = 2000;
    if(GetOpenFileName(&ofn)) 
    {
        std::wstring ext = PathFindExtensionW(ofn.lpstrFile);
        
        std::transform(ext.begin(), ext.end(), ext.begin(), towupper);

        if(ext == L".LNK")
        {
            
            // launch using shortcut method
            HRESULT hres = 0;
            IShellLinkW* psl = 0;

            // Get a pointer to the IShellLink interface.
            hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&psl));

            if (SUCCEEDED(hres))
            { 
                
                IPersistFile* ppf = 0;

                hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));

                if (SUCCEEDED(hres))
                { 
                    PROCESS_INFORMATION process;
                    STARTUPINFO start;
                    memset(&start,0,sizeof(start));
                    start.cb = sizeof(start);

                    ppf->Load(ofn.lpstrFile, 0);

                    wchar_t fullpath[INFOTIPSIZE];
                    wchar_t args[INFOTIPSIZE];
                    wchar_t cwd[INFOTIPSIZE];
                    psl->GetPath(fullpath, INFOTIPSIZE, 0, SLGP_UNCPRIORITY);
                    psl->GetArguments(args, INFOTIPSIZE);
                    psl->GetWorkingDirectory(cwd, INFOTIPSIZE);

                    wchar_t *pcwd = wcslen(cwd) > 0 ? cwd : 0;
                    std::wstring cmdline = std::wstring(fullpath) + L" " + std::wstring(args);

                    if(CreateProcessW(0, (LPWSTR)cmdline.c_str(),0,0,FALSE,DEBUG_ONLY_THIS_PROCESS|IDLE_PRIORITY_CLASS,0,pcwd,&start,&process) == 0)
                        return false;
                    else
                    {
                        pi = process;
                        return true;
                    }

                    ppf->Release();
                } 
                psl->Release();

            }
        }

        PROCESS_INFORMATION process;
        STARTUPINFO start;
        memset(&start,0,sizeof(start));
        start.cb = sizeof(start);

        basic_string<wchar_t> dir = GetDirFromString(ofn.lpstrFile);

        if(dir.empty())
        {
            if(CreateProcessW(0,ofn.lpstrFile,0,0,FALSE,DEBUG_ONLY_THIS_PROCESS|IDLE_PRIORITY_CLASS,0,0,&start,&process) == 0)
                return false;
        }
        else
        {
            if(CreateProcessW(0,ofn.lpstrFile,0,0,FALSE,DEBUG_ONLY_THIS_PROCESS|IDLE_PRIORITY_CLASS,0,dir.c_str(),&start,&process) == 0)
                return false;
        }
        pi = process;
        return true;
    }

    return false;
}

static void getCurrentDir()
{
    GetCurrentDirectory(sizeof(currentDir)/sizeof(wchar_t), currentDir);
}

//write the current directory to registry
static void writeRegistry()
{
    RegKey key(HKEY_CURRENT_USER);
    key.Create(REGISTRY_DIR);
    key[L"path"] = currentDir;
}

static void showSplashScreen(HINSTANCE hInstance)
{
#if defined(SPLASH_SCREEN)
    CSplash splash;
    HBITMAP pic = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_SPLASH));
    splash.SetBitmap(pic);
    splash.ShowSplash();
    Sleep(5000);
    splash.CloseSplash();
#endif
}

inline vector<DWORD> GetPIDs(const wchar_t *processName)
{
    vector<DWORD> pids;

    HANDLE th = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(th != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);


        BOOL next = Process32First(th, &pe);

        while(next)
        {
            if(std::wstring(PathFindFileName(pe.szExeFile)) == processName)
            {
                pids.push_back(pe.th32ProcessID);
            }
            next = Process32Next(th, &pe);
        }
        CloseHandle(th);
    }

    return pids;
}

inline void Tokenize(const string & str, vector<string> & tokens, const string & delimiters = " ")
{
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(delimiters, pos);
        pos = str.find_first_of(delimiters, lastPos);
    }
}

inline string Join(const vector<string > & seq, const string & seperator)
{
    vector<string>::size_type len = seq.size(), i;

    if ( len == 0 ) 
        return "";

    if ( len == 1 ) 
        return seq[0];

    string res( seq[0] );

    for ( i = 1; i < len; ++i )
    {
        res += seperator + seq[i];
    }


    return res;
}

inline wstring ExePath() 
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW( NULL, buffer, MAX_PATH );
    wstring::size_type pos = wstring( buffer ).find_last_of( L"\\/" );
    return wstring( buffer ).substr( 0, pos);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd )
{
    CoInitialize(0);

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};

    vector<string> tokens;

    

    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;

    getCurrentDir();

    writeRegistry();

    showSplashScreen(hInstance);

    wchar_t dllName[] = DLL_NAME;
    wchar_t *dllPath = new wchar_t[wcslen(currentDir) + sizeof(dllName)/sizeof(wchar_t) + 2];
    wcscpy(dllPath, currentDir);
    wcscat(dllPath, dllName);

    std::wifstream processFile(L"injector.txt");
    bool hasProcessName = false;
    if(processFile.good())
    {
        std::wstring processName;
        if (std::getline(processFile, processName))
        {
            if(!processName.empty())
            {
                vector<DWORD> pids = GetPIDs(processName.c_str());
                if(!pids.empty())
                {
                    getCurrentDir();
                    for (size_t i = 0; i < pids.size(); i++)
                    {
                        DWORD pid = pids[i];
                        wchar_t dllName[] = DLL_NAME;
                        wchar_t *dllPath = new wchar_t[wcslen(currentDir) + sizeof(dllName) / sizeof(wchar_t)+2];
                        wcscpy(dllPath, currentDir);
                        wcscat(dllPath, dllName);
                        HANDLE hp = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
                        if (hp == 0 || hp == INVALID_HANDLE_VALUE)
                        {
                            MessageBoxW(0, L"Failed to inject", L"FAILED", 0);
                        }
                        else
                        {
                            hasProcessName = true;
                            InjectDLL_RemoteThread(hp, dllPath);
                            CloseHandle(hp);
                        }
                    }
                }
                else
                {
                    MessageBoxW(0, (std::wstring(L"Cant find ") + processName).c_str(), L"Error", 0);
                }
            }
        }
    }
    if(!hasProcessName)
    {
        Tokenize(lpCmdLine, tokens, " ");
        if(tokens.empty())
        {
            if(openFile(pi))
            {
                UINT_PTR entry = GetEntryPointOfImage(pi.hProcess);
                BreakpointEntry(pi.hProcess, entry);
                InjectOnEntryPoint(pi.dwProcessId, pi.hProcess, entry, dllPath);
                if(IsWindows2000())
                {
                    KeepDebuggingUntilExit();
                }
                else
                {
                    DebugActiveProcessStop(pi.dwProcessId);
                }
                /*ResumeThread(pi.hThread);
                Sleep(5000);
                InjectDLL_RemoteThread(pi.hProcess, dllPath);*/
            
            }
            //else
            //    MessageBoxW(NULL, L"Can't open file", L"ERROR", NULL);
        }
        else
        {
            string command = Join(tokens, " ");
            string currentDir = GetDirFromString(command);
            size_t commandLen = command.length() + 1;
            size_t currentDirLen = currentDir.length() + 1;

            wchar_t *commandW = new wchar_t[commandLen];
            wchar_t *currentDirW = currentDir.empty() ? 0 : new wchar_t[currentDirLen];

            if(currentDirW)
                MultiByteToWideChar(CP_UTF8, 0, currentDir.c_str(), -1, currentDirW, (int)currentDirLen);
        
            MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, commandW, (int)commandLen);

            BOOL success;

            std::wstring ext;

            if(wcslen(commandW) >= 4)
            {
                size_t len = wcslen(commandW);
                ext = commandW + len - 4;
                std::transform(ext.begin(), ext.end(), ext.begin(), towupper);
            }
            else
            {
                ext = L"";
            }

            if(ext == L".LNK")
            {

                // launch using shortcut method
                HRESULT hres = 0;
                IShellLinkW* psl = 0;

                // Get a pointer to the IShellLink interface.
                hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&psl));

                if (SUCCEEDED(hres))
                { 

                    IPersistFile* ppf = 0;

                    hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));

                    if (SUCCEEDED(hres))
                    { 
                        //PROCESS_INFORMATION process;
                        STARTUPINFO start;
                        memset(&start,0,sizeof(start));
                        start.cb = sizeof(start);

                        ppf->Load(commandW, 0);

                        wchar_t fullpath[INFOTIPSIZE];
                        wchar_t args[INFOTIPSIZE];
                        wchar_t cwd[INFOTIPSIZE];
                        psl->GetPath(fullpath, INFOTIPSIZE, 0, SLGP_UNCPRIORITY);
                        psl->GetArguments(args, INFOTIPSIZE);
                        psl->GetWorkingDirectory(cwd, INFOTIPSIZE);

                        wchar_t *pcwd = wcslen(cwd) > 0 ? cwd : 0;
                        std::wstring cmdline = std::wstring(fullpath) + L" " + std::wstring(args);

                        success = CreateProcessW(0, (LPWSTR)cmdline.c_str(),0,0,FALSE,DEBUG_ONLY_THIS_PROCESS|IDLE_PRIORITY_CLASS,0,pcwd,&si,&pi);

                        ppf->Release();
                    } 
                    psl->Release();

                }
            }
            else
            {
                success = CreateProcessW(0,commandW,0,0,FALSE,DEBUG_ONLY_THIS_PROCESS|IDLE_PRIORITY_CLASS,0,currentDirW,&si,&pi);
            }

            delete[] commandW;
            delete[] currentDirW;

            if(success == FALSE)
            {
                MessageBoxW(0, L"Failed to create process", L"Error", 0);
            }
            else
            {
                /*InjectDLL_RemoteThread(pi.hProcess, dllPath);
                ResumeThread(pi.hThread);*/

                UINT_PTR entry = GetEntryPointOfImage(pi.hProcess);
                BreakpointEntry(pi.hProcess, entry);
                InjectOnEntryPoint(pi.dwProcessId, pi.hProcess, entry, dllPath);
                if(IsWindows2000())
                {
                    KeepDebuggingUntilExit();
                }
                else
                {
                    DebugActiveProcessStop(pi.dwProcessId);
                }

            }

        }
    }

    

    return 0;
}

