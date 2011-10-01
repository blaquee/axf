#ifndef hookah_nt_h__
#define hookah_nt_h__

HMODULE LoadNTDLLFunctions();

//////////////////////////////////////////////////////////////////////////

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

typedef struct tag_PEB {
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


typedef NTSTATUS (NTAPI *pfnNtQueryInformationProcess)(
    IN  HANDLE ProcessHandle,
    IN  PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN  ULONG ProcessInformationLength,
    OUT PULONG ReturnLength    OPTIONAL
    );

extern pfnNtQueryInformationProcess _NtQueryInformationProcess;

#endif // hookah_nt_h__

