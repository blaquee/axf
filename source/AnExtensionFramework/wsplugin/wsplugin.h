#ifndef wsplugin_h__
#define wsplugin_h__

#include "wsexception.h"
#include "wslog.h"

// these typedefs can be used by both dll and python plugins

// data type defs
struct AxfHandle__{int unused;}; 
typedef struct AxfHandle__ *AxfHandle;

typedef int AxfBool;
typedef int Protocol;
typedef unsigned int ProtectionMode;

typedef AxfHandle LogLevel;
typedef AxfHandle AxfExtension;

// function pointer defs
typedef void (*EventFunction)(void*);
typedef AxfExtension (*ExtensionFactoryCreate)();
typedef void (*ExtensionFactoryDestroy)(AxfExtension);

// constant defs
enum {AXFFALSE=0, AXFTRUE=1};

// plugin events
#define ON_INIT_EVENT "OnInit" // a plugin subscribes to this event if it wishes to perform initialization
#define ON_FINALIZE_EVENT "OnFinalize" // used for unloading plugins

// ProtectionMode
enum 
{ 
    PROTECTION_MODE_NOACCESS = 0x1,               
    PROTECTION_MODE_READONLY = 0x2,               
    PROTECTION_MODE_READWRITE = 0x4,              
    PROTECTION_MODE_WRITECOPY = 0x8,              
    PROTECTION_MODE_EXECUTE = 0x10,                
    PROTECTION_MODE_EXECUTE_READ = 0x20,           
    PROTECTION_MODE_EXECUTE_READWRITE = 0x40,      
    PROTECTION_MODE_EXECUTE_WRITECOPY = 0x80,      
    PROTECTION_MODE_GUARD = 0x100,                 
    PROTECTION_MODE_NOCACHE = 0x200,               
    PROTECTION_MODE_WRITECOMBINE = 0x400             
};

// data struct defs
struct String
{
    char* buffer;
    size_t len;
};

struct ProcessInfo
{
    char processName[2048];
    unsigned int processID;
};

struct AllocationInfo
{
    void *base;
    size_t size;
};

struct ExtensionFactory
{
    ExtensionFactoryCreate Create;
    ExtensionFactoryDestroy Destroy;
};

struct EventFunctionData
{
    EventFunction func;
    std::string name;

    EventFunctionData(const struct _PluginInterface *pi, const std::string &name, EventFunction func)
        : name(name), func(func)
    {

    }
};

struct HardwareBreakpoint
{
    std::set<AxfHandle> hookedTids;
    void *handler[4];
    EventFunction handlerVar[4];
    void *userdata[4];
    std::unordered_map<AxfHandle, CONTEXT> threadContext;

    HardwareBreakpoint()
    {
        for (int i = 0; i < 4; i++)
        {
            handler[i] = 0;
            handlerVar[i] = 0;
            userdata[i] = 0;
        }
    }
};

// private data do not expose
struct PluginInterfaceData
{
    void *moduleHandle;  // module handle for current dll
    std::string pluginName; //current plugin name 

    std::map<std::string, std::set<EventFunctionData*> > registerEvents;
    std::set<EventFunctionData*> registerEventsCache;
    std::set<std::shared_ptr<HookState> > hookStateCache;
    
    LogInterface *log;
    std::map<AxfExtension, ExtensionFactory> extensionCache;

    std::set<AxfHandle> openedThreadHandles;

    HardwareBreakpoint hwbpData;

    ~PluginInterfaceData();
};



// these are for dll plugins
extern "C"
{

typedef struct _SystemInterface
{
    void *(*GetModuleHandle)(const struct _PluginInterface*);
    void (*GetProcessInformation)(ProcessInfo *info);
    void *(*GetModuleBase)(const char *name); /* Get base address of a module */
    void *(*GetProcAddress)(void *base, const char *name);

    /* Throws an exception to the plugin manager, dataUnused must be set to NULL for now  */
    void (*RaiseException)(const char *exceptionMsg, void *dataUnused);

    const char* (*GetAboutMessage)(void);
    unsigned int (*GetVersion)(void);

    /* Pass NULL to these functions to return the required size of the string */
    size_t (*GetBaseDirectory)(String*);
    size_t (*GetPluginDirectory)(String*);
    size_t (*GetExtensionDirectory)(String*);
} SystemInterface;

typedef struct _LoggingInterface
{
    LogLevel Quiet;
    LogLevel Info;
    LogLevel Debug;
    LogLevel Warn;
    LogLevel Error;

    void (*SetLogLevel)(const struct _PluginInterface*, const LogLevel type);
    LogLevel (*GetLogLevel)(const struct _PluginInterface*);
    void (*Log)(const struct _PluginInterface*, const char *s);
    void (*Log2)(const struct _PluginInterface*, const LogLevel type, const char *s);
    void(*LogBinaryData)(const struct _PluginInterface*, const char *title, unsigned char *buf, int len);
} LoggingInterface;

typedef struct _PluginManagerInterface
{
    size_t (*GetUnloadedPluginList)(String *strs, size_t sizeofStrs);
    size_t (*GetLoadedPluginList)(String *strs, size_t sizeofStrs);
    AxfBool (*Load)(const char* fileName);
    AxfBool (*Unload)(const char *fileName);
    AxfBool (*Reload)(const char *fileName);
    AxfBool (*UnloadSelf)(const struct _PluginInterface*);
    AxfBool (*ReloadSelf)(const struct _PluginInterface*);
} PluginManagerInterface;


typedef struct _EventInterface
{
    size_t (*GetEventList)(String *strs, size_t sizeofStrs);
    AxfBool (*IsEventAvailable)(const char *eventName);

    /* all event functions are __cdecl call convention */
    /* returns NULL handle on failure */
    AxfHandle (*SubscribeEvent)(const struct _PluginInterface*, const char *eventName, EventFunction eventFunc);

    /* You do not have to manually remove the event in OnUnload(), 
       the plugin manager will take care of cleaning up events */
    void (*UnsubscribeEvent)(const struct _PluginInterface*, AxfHandle handle);

    AxfBool (*IsEventSubscribed)(const struct _PluginInterface*, AxfHandle handle);
} EventInterface;

typedef struct _HookInterface
{
    AxfHandle (*HookFunction)(const struct _PluginInterface*, void *oldAddress, void *newAddress);
    AxfBool (*UnhookFunction)(const struct _PluginInterface*, AxfHandle handle);
    AxfBool (*IsHooked)(void *oldAddress);
    void *(*GetOriginalFunction)(AxfHandle);

    /* breakpoints */
    void (*SetBreakpointFunc)(const struct _PluginInterface*, AxfHandle threadId, unsigned int bpSlot, void *func, void *handler);
    void (*SetBreakpointVar)(const struct _PluginInterface*, AxfHandle threadId, unsigned int bpSlot, AxfBool read, AxfBool write, int size, void *varAddr, EventFunction handler, void *userdata);
    void (*ResetBreakpoint)(const struct _PluginInterface*);
    void (*DeleteBreakpoint)(const struct _PluginInterface *, AxfHandle threadId, unsigned int bpSlot);
} HookInterface;

typedef struct _MemoryInterface
{
    /*
        Passing a statically allocated memory address located in a module (EXE/DLL) will return the ImageBase address
        Passing a dynamically allocated memory address will return its memory block base address

        returns AXFFALSE if the function fails
    */
    AxfBool (*GetAllocationBase)(AllocationInfo *allocInfo, void*);

    /*
        ProtectionMode:
            PROTECTION_MODE_EXECUTE 
            PROTECTION_MODE_NOACCESS                
            PROTECTION_MODE_READONLY                
            PROTECTION_MODE_READWRITE               
            PROTECTION_MODE_WRITECOPY               
            PROTECTION_MODE_EXECUTE                 
            PROTECTION_MODE_EXECUTE_READ            
            PROTECTION_MODE_EXECUTE_READWRITE      
            PROTECTION_MODE_EXECUTE_WRITECOPY       
            PROTECTION_MODE_GUARD                  
            PROTECTION_MODE_NOCACHE             
            PROTECTION_MODE_WRITECOMBINE              

        Returns the old protection mode
    */
    ProtectionMode (*VirtualProtect)(void *address, size_t size, ProtectionMode newProtection);

    /*
        The signature is formatted as following:
        0. Null terminated string
        1. Groups of 2 characters long hexcode separated by arbitrary amount whitespaces (tab, spacebar, newline)
        2. A 2 characters question mark (??) represents a wildcard, it will be ignored during the scanning

        Examples:
        char *sig = "1F 01 00 B9 FF ?? AB ?? ?? ?? ?? EF";

        // you can also use newlines
        char *sig = "1F 01 00 B9 FA ?? AB ?? ?? ?? ?? EF"
                    "2F 02 00 B9 FB ?? AB ?? ?? ?? ?? EF"
                    "3F 03 00 B9 FC ?? AB ?? ?? ?? ?? EF";

        // you can also use irregular spacing, mixed with tabs
        char *sig = "1F      01 00 B9 FA ??     AB ?? ?? ?? ?? EF" 
                    "2F 02 00 B9 FB ?? AB ?? ??     ?? ?? EF"
                    "3F 03 00 B9 FC ?? AB               ?? ?? ?? ?? EF";

        Returns null if the function fails to find the signature
    */
    void *(*FindSignature)(const AllocationInfo *allocInfo, const char *sig);
} MemoryInterface;

// for accessing specialized extensions specific to certain applications or games
typedef struct _ExtensionInterface
{
    /* Extensions are named using the $EXTENSION_$VERSION string format 
       where $EXTENSION is the extension name, 
       followed by underscore and $VERSION is the version number (starting from 1, up to infinity)

       For example: HookInterface_1
    */

    size_t (*GetExtensionList)(String *strs, size_t sizeofStrs);
    AxfBool (*IsExtensionAvailable)(const char *name);
    AxfExtension (*GetExtension)(const struct _PluginInterface*, const char *name);
    AxfBool (*ReleaseExtension)(const struct _PluginInterface*, AxfExtension ext);
} ExtensionInterface;

typedef struct _ThreadInterface
{
    AxfHandle (*GetCurrentThread)(void);
    AxfHandle (*GetCurrentThreadId)(void);
    AxfHandle (*OpenThread)(const struct _PluginInterface*, AxfHandle threadId);
    void (*CloseThread)(const struct _PluginInterface*, AxfHandle threadHandle);
    void(*EnumerateThreads)(void(*callback)(AxfHandle threadId, AxfHandle ownerProcessId));
    AxfHandle (*GetCurrentProcess)(void);
    AxfHandle (*GetCurrentProcessId)(void);
    void(*EnumerateProcesses)(void(*callback)(AxfHandle processId));
} ThreadInterface;

typedef struct _PluginInterface
{
    PluginInterfaceData *data;

    SystemInterface *system;
    LoggingInterface *log;
    PluginManagerInterface *manager;
    EventInterface *event;
    ExtensionInterface *extension;
    HookInterface *hook;
    MemoryInterface *memory;
    ThreadInterface *thread;

} PluginInterface;

}


//////////////////////////////////////////////////////////////////////////
class PluginInterfaceWrapper
{
    PluginInterface pluginInterface;

public:
    PluginInterfaceWrapper();
    ~PluginInterfaceWrapper();

    const PluginInterface *operator->() const
    {
        return &pluginInterface;
    }

    PluginInterface *operator->()
    {
        return &pluginInterface;
    }

    operator const PluginInterface*() const 
    {
        return &pluginInterface;
    }

    operator PluginInterface*()
    {
        return &pluginInterface;
    }

    operator PluginInterface&()
    {
        return pluginInterface;
    }

    operator const PluginInterface&() const
    {
        return pluginInterface;
    }
};

class Plugin
{
    PluginInterfaceWrapper pluginInterface;
    std::string dir;
    std::string name;

public:
    Plugin(const std::string &dir, const std::string &name);
    virtual ~Plugin();

    void InternalLoad();
    void InternalUnload();

    std::string GetFilePath() const;
    const std::string &GetFileDir() const;
    const std::string &GetFileName() const;

    PluginInterface &GetPluginInterface();

public:
    // whether the plugin type supports softreload or not
    virtual bool IsSoftReloadSupported() const { return false; }
    virtual void SoftReload() { throw WSException("Soft reload is unsupported for this plugin"); }

protected:
    // subclasses must implement these functions
    virtual int Load()=0; // returns the version of the pluginapi
    virtual void Unload()=0;
    virtual AxfBool OnInit()=0; // calls the OnInit function of the plugin
};


//////////////////////////////////////////////////////////////////////////

typedef void (*OnInitType)(const PluginInterface *);

typedef struct _PluginDescription
{
    unsigned int version;  /* the version of this plugin */
    unsigned int pluginapiVersion; /* the version of the pluginapi this plugin is using, must be AXF_API_VERSION (was AXF_PLUGIN_VERSION) */

    AxfBool (*OnInit)(const struct _PluginInterface*);  /* entry point, cdecl only */

    /* optional info */ 
    const char *name;
    const char *author; 
    const char *about;  

    void *reserved0; /* must be NULL */
    void *reserved1; /* must be NULL */
} PluginDescription;

class ModuleLoaderHider
{
    HMODULE module;
    ModuleRecord modRec; // for restoring the original module record to the module list

public:
    explicit ModuleLoaderHider(const std::string &moduleName)
        : module(LoadLibraryA(moduleName.c_str()))
    {
        if(module == 0)
            throw WSException(std::string("Failed to load DLL ") + moduleName);

        modRec = UnlinkModule(module);
    }
    ~ModuleLoaderHider()
    {
        if(module)
        {
            LinkModule(module, modRec);
            FreeLibrary(module);
        }
    }
    operator HMODULE() const
    {
        return module;
    }
    operator void*() const
    {
        return (void*)module;
    }
};

class DllPlugin : public Plugin
{
    // these will be obtained from the dlls
    PluginDescription *pluginDesc;
    std::shared_ptr<ModuleLoaderHider> module;

public:
    DllPlugin( const std::string &dir, const std::string &name );
    virtual ~DllPlugin();

protected:
    virtual int Load();
    virtual void Unload();
    virtual AxfBool OnInit();
};

class CustomPlugin : public Plugin
{
    std::string ext;

    // client data
    AxfHandle clientHandle;
    void(*OnPluginUnload)(AxfHandle);

public:
    CustomPlugin( const std::string &dir, const std::string &name, const std::string &ext );
    virtual ~CustomPlugin();

protected:
    virtual int Load();
    virtual void Unload();
    virtual AxfBool OnInit();
};


#endif // wsplugin_h__
