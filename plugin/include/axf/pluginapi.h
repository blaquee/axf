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
#endif

#include <stdlib.h>
#include <stdarg.h>

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

struct _PluginDescription
{
    unsigned int version;  /* the version of this plugin */
    unsigned int pluginapiVersion; /* the version of the pluginapi this plugin is using, must be AXF_API_VERSION (was AXF_PLUGIN_VERSION) */

    void (*OnInit)(const struct _PluginInterface*);  /* entry point, cdecl only */

    /* optional info */ 
    const char *name;
    const char *author; 
    const char *about;  

    void *reserved0; /* must be NULL */
    void *reserved1; /* must be NULL */
};
typedef struct _PluginDescription PluginDescription;

/*
    Every plugin must export a pointer to a PluginDescription filled with the required info (version, pluginapiVersion and initFunction)
    either by using the convenience macro AXF_PLUGIN_DESCRIPTION() or manually exporting "PluginDescription *plugindesc"

    "version" should contain an unsigned int that is meaningful to your plugin
    "pluginapiVersion" should always be AXF_API_VERSION (any other value may cause your plugin to be incompatible with AXF)
    "initFunction" should contain a pointer to your OnInit function

    name, author and about are optional (use either NULL or a pointer to string)

    the reserved fields must be NULL
*/
#define AXF_PLUGIN_DESCRIPTION(ver, initFunction, pluginName, pluginAuthor, pluginAbout) \
    __plugindesc__.version = ver; \ 
    __plugindesc__.pluginapiVersion = AXF_API_VERSION; \
    __plugindesc__.OnInit = (initFunction == 0 ? 0 : _native_stub(&initFunction)); \
    __plugindesc__.name = pluginName; \
    __plugindesc__.author = pluginAuthor; \
    __plugindesc__.about = pluginAbout; \
    __plugindesc__.reserved0 = 0; \
    __plugindesc__.reserved1 = 0;

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

struct _ProcessInfo
{
    char processName[2048];
    unsigned int processID;

};
typedef struct _ProcessInfo ProcessInfo;

struct _AllocationInfo
{
    void *base;
    size_t size;
};
typedef struct _AllocationInfo AllocationInfo;

/************************************************************************/
/* Helper Functions                                                     */
/************************************************************************/

struct _String
{
    char* buffer;
    size_t len;
};
typedef struct _String String;

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
/*
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
*/


/************************************************************************/
/* Plugin Interface                                                     */
/************************************************************************/

struct _SystemInterface
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
};
typedef struct _SystemInterface SystemInterface;

struct _LoggingInterface
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
};
typedef struct _LoggingInterface LoggingInterface;

struct _PluginManagerInterface
{
    size_t (*GetUnloadedPluginList)(String *strs, size_t sizeofStrs);
    size_t (*GetLoadedPluginList)(String *strs, size_t sizeofStrs);
    WsBool (*Load)(const char* fileName);
    WsBool (*Unload)(const char *fileName);
    WsBool (*Reload)(const char *fileName);
};
typedef struct _PluginManagerInterface PluginManagerInterface;

struct _EventInterface
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
};
typedef struct _EventInterface EventInterface;

struct _HookInterface
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
};
typedef struct _HookInterface HookInterface;

struct _MemoryInterface
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
};
typedef struct _MemoryInterface MemoryInterface;

/* for accessing specialized extensions specific to certain applications or games*/
struct _ExtensionInterface
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
};
typedef struct _ExtensionInterface ExtensionInterface;

struct _PluginInterface
{
    const void * const data;

    const SystemInterface * const system;
    const LoggingInterface * const log;
    const PluginManagerInterface * const manager;
    const EventInterface * const event;
    const ExtensionInterface * const extension;
    const HookInterface * const hook;
    const MemoryInterface * const memory;

};
typedef struct _PluginInterface PluginInterface;

#ifdef __cplusplus
}
#endif


/************************************************************************/
/* C++ Interface                                                        */
/************************************************************************/

#ifdef __cplusplus


#endif /* #ifdef __cplusplus */

#endif


