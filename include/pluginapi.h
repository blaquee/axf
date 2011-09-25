/*

Copyright (c) 2009, Hunter and genuine (http://mp.reversing.us)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the MPReversing nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY Hunter and genuine (http://mp.reversing.us) ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Hunter and genuine (http://mp.reversing.us) BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef pluginapi_h__
#define pluginapi_h__

#if defined(WIN32) || defined(_WIN32)

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#endif

#ifdef __cplusplus
#include <string>
#include <vector>
#include <algorithm>
#include <exception>
#endif


#ifdef __cplusplus
extern "C"
{
#endif

/*
How the AXF plugin version works

The overall version (AXF_PLUGIN_VERSION) is formed by joining 3 segments up (each part is described below).

Each segment ranges from 0 to 255.

if one of the segment increases, all the segments below it must be reset to 0
for example an increase to minor version done like this: 1.0.50 becomes 1.1.0

major = Will only be increased if there are major changes to the system that will 
render all existing plugins completely unusable (a plugin rewrite is unavoidable).
At the moment, increasing the major version is reserved for a complete codebase rewrite (which I hope will never happen).

minor = Small breaking changes to the system or API that may or may not require the plugins to be updated.

service = Bug fixes or enhancements to the system, no API changes and will not require plugin update.

*/

#define AXF_API_MAJOR_VERSION 1
#define AXF_API_MINOR_VERSION 0
#define AXF_API_SERVICE_VERSION 0

#define AXF_API_VERSION ( ((AXF_API_MAJOR_VERSION) << (16)) | \
                             ((AXF_API_MINOR_VERSION) << (8))  |  \
                              (AXF_API_SERVICE_VERSION) )

    
/************************************************************************/
/* Plugin Description and Events                                        */
/************************************************************************/

#if defined(WIN32) || defined(_WIN32)
    #undef AXF_EXPORT

    #ifdef __cplusplus
        #define AXF_EXPORT extern "C" __declspec(dllexport)
    #else
        #define AXF_EXPORT __declspec(dllexport) 
    #endif
#else
    #undef AXF_EXPORT
    #define AXF_EXPORT
#endif

typedef struct _PluginDescription
{
    unsigned int version:32;  /* the version of this plugin */
    unsigned int pluginapiVersion:32; /* the version of the pluginapi this plugin is using, must be AXF_API_VERSION (was AXF_PLUGIN_VERSION) */

    void (*OnInit)(const struct _PluginInterface*);  /* entry point, cdecl only */

    /* optional info */ 
    const char *name;
    const char *author; 
    const char *about;  

    void *reserved0; /* must be NULL */
    void *reserved1; /* must be NULL */
} PluginDescription;

/*
    Every plugin must export a pointer to a PluginDescription filled with the required info (version, pluginapiVersion and initFunction)
    either by using the convenience macro AXF_PLUGIN_DESCRIPTION() or manually exporting "PluginDescription *plugindesc"

    "version" should contain an unsigned int that is meaningful to your plugin
    "pluginapiVersion" should always be AXF_API_VERSION (any other value may cause your plugin to be incompatible with AXF)
    "initFunction" should contain a pointer to your OnInit function

    name, author and about are optional (use either NULL or a pointer to string)

    the reserved fields must be NULL
*/
#define AXF_PLUGIN_DESCRIPTION(version, initFunction, name, author, about) \
    static PluginDescription plugindesc_ = { version, AXF_API_VERSION, &initFunction, name, author, about, 0, 0 }; \
    AXF_EXPORT PluginDescription *plugindesc = &plugindesc_;


/* The following functions must be implemented with C (__cdecl) calling convention */

/* called when the plugin gets loaded
 the plugin cannot be loaded if this function isn't implemented 
 */
static void OnInit(const struct _PluginInterface *);


/* These functions may be added to the event manager via the EventInterface */

/* called when the plugin gets unloaded
   the plugin cannot be unloaded if this function isn't implemented 
   to be used with ON_FINALIZE_EVENT
*/
static void OnFinalize(void *unused);


/************************************************************************/
/* Events                                                               */
/************************************************************************/
/* Event types used for Adding events using EventInterface::AddEvent */
#define ON_FINALIZE_EVENT "OnFinalize" /* subscribe to this event if you want your plugin to be unloadable*/


/************************************************************************/
/* Data Types                                                           */
/************************************************************************/

typedef void (*EventFunction)(void*);
struct WsHandle__{int unused;}; 
typedef struct WsHandle__ *WsHandle;
typedef int WsBool;
typedef unsigned int ProtectionMode;

typedef const WsHandle LogLevel;
typedef WsHandle WsExtension;

enum {WSFALSE=0, WSTRUE=1};

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

typedef struct _ProcessInfo
{
    char processName[2048];
    unsigned int processID;

} ProcessInfo;

typedef struct _AllocationInfo
{
    void *base;
    size_t size;
} AllocationInfo;


/************************************************************************/
/* Helper Functions                                                     */
/************************************************************************/

typedef struct _String
{
    char* buffer;
    size_t len;
} String;


/* Allocates String Struct and allocates memory for the string and its length */
static String* AllocString(size_t strlength, size_t strSize)
{
    size_t i;
    String* newStr = (String*)malloc(strSize*sizeof(String));
    for( i = 0; i < strSize; i++)
    {
        newStr[i].buffer = (char*)malloc(strlength+sizeof(char));
        newStr[i].len = strlength;
    }
    return newStr;
}

static void FreeString(String* delStr, size_t strSize)
{
    size_t i;
    if(delStr!=NULL)
    {
        for(i = 0; i < strSize; i ++)
        {
            free(delStr[i].buffer);
        }
        free(delStr);

    }
}

static String *NullString()
{
    String *s = AllocString(1, 1);
    s->buffer[0] = '\0';

    return s;
}

static void FreeString1(String *s)
{
    FreeString(s, 1);
}


static String *ExtendString(String *oldStr, size_t extraSize)
{
    size_t newSize = extraSize+oldStr->len;
    oldStr->buffer = (char*)realloc(oldStr->buffer, newSize);
    oldStr->len = newSize;
    return oldStr;
}

static void StringSprintf(String *s, const char *fmt, ...)
{
    va_list ap;
    size_t len, oldLen;

    va_start(ap, fmt);
    len = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if(len > 0)
    {
        oldLen = s->len;
        ExtendString(s, len);

        va_start(ap, fmt);
        vsnprintf(s->buffer+oldLen-1, len+1, fmt, ap);
        va_end(ap);
    }
}



/************************************************************************/
/* Plugin Interface                                                     */
/************************************************************************/

typedef struct _SystemInterface
{
    void *(*GetModuleHandle)(const struct _PluginInterface*);
    void (*GetProcessInformation)(ProcessInfo *info);
    void *(*GetModuleBase)(const char *name); /* Get base address of a module */
    void *(*GetProcAddress)(void *base, const char *name); /* Win API free version */

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
    /*
        Sometime the hook may fail even though a valid handle (non-null) is returned
        Be sure to check that your hook works
        
        Any hooked functions will get unhooked after OnUnload() has been called
    */
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

        Returns the old protection mode,
        Returns PROTECTION_MODE_NOACCESS if the function fails
    */
    ProtectionMode (*VirtualProtect)(void *address, size_t size, ProtectionMode newProtection);

    /*
        The signature is formatted as following:
        1. Groups of 2 characters long hexcode separated by arbitrary amount whitespaces (tab, spacebar, newline)
        2. a 2 characters question mark (??) represents a wildcard, it will be ignored during the scanning

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

/* for accessing specialized extensions specific to certain applications or games*/
typedef struct _ExtensionInterface
{
    /* Extensions are named using the $EXTENSION_$VERSION string format 
       where $EXTENSION is the extension name, 
       followed by underscore and $VERSION is the version number (starting from 1, up to infinity)

       For example: HookInterface_1

       Any unreleased extensions will get released after OnUnload() has been called
    */
    size_t (*GetExtensionList)(String *strs, size_t sizeofStrs);
    WsBool (*IsExtensionAvailable)(const char *name);
    WsExtension (*GetExtension)(const struct _PluginInterface*, const char *name);
    WsBool (*ReleaseExtension)(const struct _PluginInterface*, WsExtension ext);
} ExtensionInterface;

typedef struct _PluginInterface
{
    const void * const data;

    const SystemInterface * const system;
    const LoggingInterface * const log;
    const PluginManagerInterface * const manager;
    const EventInterface * const event;
    const ExtensionInterface * const extension;
    const HookInterface * const hook;
    const MemoryInterface * const memory;

} PluginInterface;


#ifdef __cplusplus
}
#endif


/************************************************************************/
/* C++ Interface                                                        */
/************************************************************************/

#ifdef __cplusplus

inline std::string StringToStdString(const String &s)
{
    return s.buffer;
}

class InterfaceEx
{
    const PluginInterface *pi;

    void ThrowIfNotInitialized() const
    {
        if(pi == 0)
            throw std::runtime_error("PluginInterfaceEx has not been initialized properly");
    }

public:
    InterfaceEx():pi(0){}
    InterfaceEx(const PluginInterface *pi) : pi(pi) {}
    virtual ~InterfaceEx() {}

    const PluginInterface &GetPluginInterface() const 
    { 
        ThrowIfNotInitialized();

        return *pi; 
    }
    const PluginInterface *GetPluginInterfacePtr() const 
    { 
        ThrowIfNotInitialized();

        return pi; 
    }

    void SetPluginInterface(const PluginInterface *pi)
    {
        this->pi = pi;
    }

    operator const PluginInterface*() const 
    {
        ThrowIfNotInitialized();

        return pi;
    }

    operator const PluginInterface&() const
    {
        ThrowIfNotInitialized();

        return *pi;
    }
};
class SystemInterfaceEx : public virtual InterfaceEx
{
    std::string GetDirectory(size_t(*GetDirFunc)(String*)) const 
    {
        size_t len = GetDirFunc(0);
        String *s = AllocString(len, 1);
        GetDirFunc(s);
        std::string res = s->buffer;
        FreeString1(s);

        return res;
    }

public:
    SystemInterfaceEx(){}
    SystemInterfaceEx(const PluginInterface *pi) : InterfaceEx(pi) {}
    virtual ~SystemInterfaceEx(){}

    void* GetModuleHandle() const
    {
        return GetPluginInterface().system->GetModuleHandle(GetPluginInterfacePtr());
    }
    ProcessInfo GetProcessInformation() const
    {
        ProcessInfo info;
        GetPluginInterface().system->GetProcessInformation(&info);

        return info;
    }
    void *GetModuleBase(const std::string &name) const
    {
        return GetPluginInterface().system->GetModuleBase(name.c_str());
    }
    void *GetProcAddress(void *base, const std::string &name) const
    {
        return GetPluginInterface().system->GetProcAddress(base, name.c_str());
    }

    /* Throws an exception to the plugin manager, dataUnused must be set to NULL for now  */
    void RaiseException(const std::string &exceptionMsg="Unknown Exception Raised!", void *dataUnused=0) const
    {
        GetPluginInterface().system->RaiseException(exceptionMsg.c_str(), dataUnused);
    }

    std::string GetAboutMessage() const
    {
        return GetPluginInterface().system->GetAboutMessage();
    }
    unsigned int GetVersion() const
    {
        return GetPluginInterface().system->GetVersion();
    }


    std::string  GetBaseDirectory() const
    {
        return GetDirectory(GetPluginInterface().system->GetBaseDirectory);
        
    }
    std::string  GetPluginDirectory() const
    {
        return GetDirectory(GetPluginInterface().system->GetPluginDirectory);
    }
    std::string  GetExtensionDirectory() const
    {
        return GetDirectory(GetPluginInterface().system->GetExtensionDirectory);
    }
};

class LoggingInterfaceEx : public virtual InterfaceEx
{
public:
    LoggingInterfaceEx(){}
    LoggingInterfaceEx(const PluginInterface *pi) : InterfaceEx(pi){}
    virtual ~LoggingInterfaceEx(){}

    LogLevel Quiet() const
    {
        return GetPluginInterface().log->Quiet;
    }
    LogLevel Debug() const
    {
        return GetPluginInterface().log->Debug;
    }
    LogLevel Info() const
    {
        return GetPluginInterface().log->Info;
    }
    LogLevel Warn() const
    {
        return GetPluginInterface().log->Warn;
    }
    LogLevel Error() const
    {
        return GetPluginInterface().log->Error;
    }

    void SetLogLevel(const LogLevel type) const
    {
        GetPluginInterface().log->SetLogLevel(GetPluginInterfacePtr(), type);
    }
    LogLevel GetLogLevel() const
    {
        return GetPluginInterface().log->GetLogLevel(GetPluginInterfacePtr());
    }
    void Log(const std::string &s) const
    {
        GetPluginInterface().log->Log(GetPluginInterfacePtr(), s.c_str());
    }
    void Log(const LogLevel type, const std::string &s) const
    {
        GetPluginInterface().log->Log2(GetPluginInterfacePtr(), type, s.c_str());
    }
};

class PluginManagerInterfaceEx : public virtual InterfaceEx
{

    std::vector<std::string> GetPluginList(size_t (*GetPList)(String *strs, size_t sizeofStrs)) const
    {
        size_t sizeOfStrs = GetPList(0, 0);
        String *strs = AllocString(2048, sizeOfStrs);
        GetPList(strs, sizeOfStrs);

        std::vector<std::string> results;
        results.resize(sizeOfStrs);
        std::transform(strs, strs+sizeOfStrs, results.begin(), &StringToStdString);

        FreeString(strs, sizeOfStrs);

        return results;
    }

public:
    PluginManagerInterfaceEx(){}
    PluginManagerInterfaceEx(const PluginInterface *pi) : InterfaceEx(pi) {}
    virtual ~PluginManagerInterfaceEx(){}

    std::vector<std::string> GetUnloadedPluginList() const
    {
        return GetPluginList(GetPluginInterface().manager->GetUnloadedPluginList);
    }
    std::vector<std::string> GetLoadedPluginList() const
    {
        return GetPluginList(GetPluginInterface().manager->GetLoadedPluginList);
    }

    WsBool LoadPlugin(const std::string &fileName) const
    {
        return GetPluginInterface().manager->Load(fileName.c_str());
    }
    WsBool UnloadPlugin(const std::string &fileName) const
    {
        return GetPluginInterface().manager->Unload(fileName.c_str());
    }
    WsBool ReloadPlugin(const std::string &fileName) const
    {
        return GetPluginInterface().manager->Reload(fileName.c_str());
    }
};

class EventInterfaceEx : public virtual InterfaceEx
{
    
public:
    EventInterfaceEx(){}
    EventInterfaceEx(const PluginInterface *pi) : InterfaceEx(pi) {}
    virtual ~EventInterfaceEx(){}

    std::vector<std::string> GetEventList() const
    {
        size_t sizeOfStrs = GetPluginInterface().event->GetEventList(0, 0);
        String *strs = AllocString(2048, sizeOfStrs);
        GetPluginInterface().event->GetEventList(strs, sizeOfStrs);

        std::vector<std::string> results;
        results.resize(sizeOfStrs);
        std::transform(strs, strs+sizeOfStrs, results.begin(), &StringToStdString);

        FreeString(strs, sizeOfStrs);

        return results;
    }
    WsBool IsEventAvailable(const std::string &eventName) const
    {
        return GetPluginInterface().event->IsEventAvailable(eventName.c_str());
    }

    /* all event functions are __cdecl call convention */
    /* returns NULL handle on failure */
    WsHandle SubscribeEvent(const std::string &eventName, EventFunction eventFunc) const
    {
        return GetPluginInterface().event->SubscribeEvent(GetPluginInterfacePtr(), eventName.c_str(), eventFunc);
    }

    /* You do not have to manually remove the event in OnUnload(), 
       the plugin manager will take care of cleaning up events */
    void UnsubscribeEvent(WsHandle handle) const
    {
        GetPluginInterface().event->UnsubscribeEvent(GetPluginInterfacePtr(), handle);
    }

    WsBool IsEventSubscribed(WsHandle handle) const
    {
        return GetPluginInterface().event->IsEventSubscribed(GetPluginInterfacePtr(), handle);
    }
};

class HookInterfaceEx : public virtual InterfaceEx
{
public:
    HookInterfaceEx(){}
    HookInterfaceEx(const PluginInterface *pi) : InterfaceEx(pi) {}
    virtual ~HookInterfaceEx(){}

    WsHandle HookFunction(void *oldAddress, void *newAddress) const
    {
        return GetPluginInterface().hook->HookFunction(GetPluginInterfacePtr(), oldAddress, newAddress);
    }
    WsBool UnhookFunction(WsHandle handle) const
    {
        return GetPluginInterface().hook->UnhookFunction(GetPluginInterfacePtr(), handle);
    }
    WsBool IsHooked(void *oldAddress) const
    {
        return GetPluginInterface().hook->IsHooked(oldAddress);
    }
    void *GetOriginalFunction(WsHandle handle) const
    {
        return GetPluginInterface().hook->GetOriginalFunction(handle);
    }
};

class MemoryInterfaceEx : public virtual InterfaceEx
{
public:
    MemoryInterfaceEx(){}
    MemoryInterfaceEx(const PluginInterface *pi) : InterfaceEx(pi) {}
    virtual ~MemoryInterfaceEx(){}

    std::unique_ptr<AllocationInfo> GetAllocationBase(void *addr) const
    {
        std::unique_ptr<AllocationInfo> allocInfo(new AllocationInfo);
        WsBool isgood = GetPluginInterface().memory->GetAllocationBase(allocInfo.get(), addr);
        if(isgood)
        {
            return allocInfo;
        }
        else
        {
            allocInfo.reset(0);
            return allocInfo;
        }
    }

    ProtectionMode VirtualProtect(void *address, size_t size, ProtectionMode newProtection) const
    {
        return GetPluginInterface().memory->VirtualProtect(address, size, newProtection);
    }

    /*
        The signature is formatted as following:
        1. Groups of 2 characters long hexcode separated by arbitrary amount whitespaces (tab, spacebar, newline)
        2. a 2 characters question mark (??) represents a wildcard, it will be ignored during the scanning

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
    void *FindSignature(const AllocationInfo *allocInfo, const char *sig) const
    {
        return GetPluginInterface().memory->FindSignature(allocInfo, sig);
    }
};

class ExtensionInterfaceEx : public virtual InterfaceEx
{
public:
    ExtensionInterfaceEx(){}
    ExtensionInterfaceEx(const PluginInterface *pi) : InterfaceEx(pi) {}
    virtual ~ExtensionInterfaceEx(){}

    std::vector<std::string> GetExtensionList() const
    {
        size_t sizeOfStrs = GetPluginInterface().extension->GetExtensionList(0, 0);
        String *strs = AllocString(2048, sizeOfStrs);
        GetPluginInterface().extension->GetExtensionList(strs, sizeOfStrs);

        std::vector<std::string> results;
        results.resize(sizeOfStrs);
        std::transform(strs, strs+sizeOfStrs, results.begin(), &StringToStdString);

        FreeString(strs, sizeOfStrs);

        return results;
    }

    WsBool IsExtensionAvailable(const std::string &extName) const
    {
        return GetPluginInterface().extension->IsExtensionAvailable(extName.c_str());
    }
    template<typename T>
    T* GetExtension(const std::string &extName) const
    {
        return (T*)GetPluginInterface().extension->GetExtension(GetPluginInterfacePtr(), extName.c_str());
    }

    WsBool ReleaseExtension(WsExtension ext) const
    {
        return GetPluginInterface().extension->ReleaseExtension(GetPluginInterfacePtr(), ext);
    }
};

class PluginInterfaceEx : public virtual InterfaceEx,
                          public EventInterfaceEx, 
                          public LoggingInterfaceEx,
                          public PluginManagerInterfaceEx, 
                          public SystemInterfaceEx,
                          public ExtensionInterfaceEx,
                          public HookInterfaceEx,
                          public MemoryInterfaceEx
{
public:
    PluginInterfaceEx(){}

    PluginInterfaceEx(const PluginInterface *pi) : 
        InterfaceEx(pi), 
        EventInterfaceEx(pi),
        LoggingInterfaceEx(pi),
        PluginManagerInterfaceEx(pi),
        SystemInterfaceEx(pi),
        ExtensionInterfaceEx(pi),
        HookInterfaceEx(pi),
        MemoryInterfaceEx(pi)
        {}

    PluginInterfaceEx &operator=(const PluginInterface *pi)
    {
        this->SetPluginInterface(pi);
        return *this;
    }
};
#endif /* #ifdef __cplusplus */

#endif


