#ifndef wsplugin_h__
#define wsplugin_h__

#include "wsexception.h"
#include "wslog.h"

// these typedefs can be used by both dll and python plugins

// data type defs
struct WsHandle__{int unused;}; 
typedef struct WsHandle__ *WsHandle;

typedef int WsBool;
typedef int Protocol;
typedef unsigned int ProtectionMode;

typedef WsHandle LogLevel;
typedef WsHandle WsExtension;

// function pointer defs
typedef void (*EventFunction)(void*);
typedef WsExtension (*ExtensionFactoryCreate)();
typedef void (*ExtensionFactoryDestroy)(WsExtension);

// constant defs
enum {WSFALSE=0, WSTRUE=1};

#define ON_UNLOAD_EVENT "OnUnload"

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

// private data do not expose
struct PluginInterfaceData
{
    void *moduleHandle;  // module handle for current dll

    std::map<std::string, std::set<EventFunctionData*> > registerEvents;
    std::set<EventFunctionData*> registerEventsCache;
    std::set<std::shared_ptr<HookState> > hookStateCache;
    
    LogInterface *log;
    std::map<WsExtension, ExtensionFactory> extensionCache;

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
} SystemInterface;

typedef struct _LoggingInterface
{
    LogLevel Quiet;
    LogLevel Debug;
    LogLevel Info;
    LogLevel Warn;
    LogLevel Error;

    void (*SetLogLevel)(const struct _PluginInterface*, const LogLevel type);
    LogLevel (*GetLogLevel)(const struct _PluginInterface*);
    void (*Log)(const struct _PluginInterface*, const char *s);
    void (*Log2)(const struct _PluginInterface*, const LogLevel type, const char *s);
} LoggingInterface;

typedef struct _PluginManagerInterface
{
    size_t (*GetUnloadedPluginList)(String *strs, size_t sizeofStrs);
    size_t (*GetLoadedPluginList)(String *strs, size_t sizeofStrs);
    WsBool (*Load)(const char* fileName);
    WsBool (*Unload)(const char *fileName);
    WsBool (*Reload)(const char *fileName);
} PluginManagerInterface;


typedef struct _EventInterface
{
    size_t (*GetEventList)(String *strs, size_t sizeofStrs);
    WsBool (*IsEventAvailable)(const char *eventName);

    /* all event functions are __cdecl call convention */
    /* returns NULL handle on failure */
    WsHandle (*SubscribeEvent)(const struct _PluginInterface*, const char *eventName, EventFunction eventFunc);

    /* You do not have to manually remove the event in OnUnload(), 
       the plugin manager will take care of cleaning up events */
    void (*UnsubscribeEvent)(const struct _PluginInterface*, WsHandle handle);

    WsBool (*IsEventSubscribed)(const struct _PluginInterface*, WsHandle handle);
} EventInterface;

typedef struct _HookInterface
{
    WsHandle (*HookFunction)(const struct _PluginInterface*, void *oldAddress, void *newAddress);
    WsBool (*UnhookFunction)(const struct _PluginInterface*, WsHandle handle);
    WsBool (*IsHooked)(void *oldAddress);
    void *(*GetOriginalFunction)(WsHandle);
} HookInterface;

typedef struct _MemoryInterface
{
    /*
        Passing a statically allocated memory address located in a module (EXE/DLL) will return the ImageBase address
        Passing a dynamically allocated memory address will return its memory block base address

        returns WSFALSE if the function fails
    */
    WsBool (*GetAllocationBase)(AllocationInfo *allocInfo, void*);

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
    WsBool (*IsExtensionAvailable)(const char *name);
    WsExtension (*GetExtension)(const struct _PluginInterface*, const char *name);
    WsBool (*ReleaseExtension)(const struct _PluginInterface*, WsExtension ext);
} ExtensionInterface;

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
    virtual int Load()=0; // returns the version of the plugin
    virtual void Unload()=0;
};


//////////////////////////////////////////////////////////////////////////

typedef int (*OnLoadType)(const PluginInterface *);

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
    OnLoadType OnLoad;
    std::shared_ptr<ModuleLoaderHider> module;


public:
    DllPlugin( const std::string &dir, const std::string &name );
    virtual ~DllPlugin();

protected:
    virtual int Load();
    virtual void Unload();

};


#endif // wsplugin_h__
