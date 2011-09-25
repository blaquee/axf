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

#ifndef extensionapi_h__
#define extensionapi_h__

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

#include "pluginapi.h"

#ifdef __cplusplus
extern "C"
{
#endif
    
/************************************************************************/
/* Plugin Description and Events                                        */
/************************************************************************/

typedef struct _ExtensionDescription
{
    unsigned int version:32;  /* the version of this plugin */
    unsigned int pluginapiVersion:32; /* the version of the pluginapi this plugin is using, must be AXF_API_VERSION (was AXF_PLUGIN_VERSION) */

    void (*OnInit)(const struct _PluginInterface*, const struct _ExtenderInterface*);  /* entry point, cdecl only */

    /* optional info */ 
    const char *name;
    const char *author; 
    const char *about;  

    void *reserved0; /* must be NULL */
    void *reserved1; /* must be NULL */
} ExtensionDescription;

/*
    Every plugin must export a pointer to a PluginDescription filled with the required info (version, pluginapiVersion and initFunction)
    either by using the convenience macro AXF_PLUGIN_DESCRIPTION() or manually exporting "PluginDescription *plugindesc"

    "version" should contain an unsigned int that is meaningful to your plugin
    "pluginapiVersion" should always be AXF_API_VERSION (any other value may cause your plugin to be incompatible with AXF)
    "initFunction" should contain a pointer to your OnInit function

    name, author and about are optional (use either NULL or a pointer to string)

    the reserved fields must be NULL
*/
#define AXF_EXTENSION_DESCRIPTION(version, initFunction, name, author, about) \
    static ExtensionDescription extdesc_ = { version, AXF_API_VERSION, &initFunction, name, author, about, 0, 0 }; \
    AXF_EXPORT ExtensionDescription *extdesc = &extdesc_;


/* The following functions must be implemented with C (__cdecl) calling convention */

/* called when the plugin gets loaded
 the plugin cannot be loaded if this function isn't implemented 
 */
static void OnInitExtension(const struct _PluginInterface *, const struct _ExtenderInterface*);



/************************************************************************/
/* Events                                                               */
/************************************************************************/
/* Event types used for Adding events using EventInterface::AddEvent */

/* 
    for loading of custom plugin types
    callback prototype is void OnLoadPlugin(const PluginData*);
*/
#define ON_LOAD_PLUGIN "OnLoadPlugin"  

/************************************************************************/
/* Data Types                                                           */
/************************************************************************/
typedef WsExtension (*ExtensionFactoryCreate)();
typedef void (*ExtensionFactoryDestroy)(WsExtension);

typedef struct _ExtensionFactory
{
    ExtensionFactoryCreate Create;
    ExtensionFactoryDestroy Destroy;
} ExtensionFactory;

/* Used for the ON_LOAD_PLUGIN event */
typedef struct _PluginBinary
{
    const unsigned char *data;
    unsigned int size;
} PluginBinary;
typedef struct _PluginData
{
    /* the extension sets this to AXF_PLUGIN_VERSION */
    int clientVersion; 

    /* sets this to non-null to signify a successful load, it can be a pointer of any type */
    WsHandle clientHandle; 

    /* set this function to non-null to signify a successful load, the WsHandle argument is the clientHandle */
    void(*OnPluginUnload)(WsHandle); 

    /* PluginData inputs */
    const char *name;
    const char *extension;
    WsBool (*GetBinary)(struct _PluginData*, PluginBinary*);
    void (*ReleaseBinary)(PluginBinary*);
} PluginData;

/*
    Param1: Log Level
    Param2: Input string 
*/
typedef void (*LogOutputFunc)(LogLevel ,const char *);

/*
    Param1: Output string
    Param2: Input Log level
    Param3: Input string
*/
typedef void (*LogFormatterFunc)(String **, LogLevel, const char *);

/*
    Param1: Input log level
    Param2: Input string

    Return: WSTRUE to accept the input string, otherwise reject it
*/
typedef WsBool (*LogFilterFunc)(LogLevel, const char *);

/************************************************************************/
/* Extender Interface                                                   */
/************************************************************************/

typedef struct _EventExtenderInterface
{
    void (*AddEvent)(const char *name);
    void (*FireEvent)(const char *name, void *data);
}EventExtenderInterface;

typedef struct _ExtensionExtenderInterface
{
    void (*AddExtension)(const char *name, const ExtensionFactory *fac);
}ExtensionExtenderInterface;

typedef struct _LogExtenderInterface
{
    void (*AddLogger)(LogLevel, LogOutputFunc, LogFormatterFunc, LogFilterFunc);
}LogExtenderInterface;

typedef struct _ExtenderInterface
{
    const void * const data;

    const EventExtenderInterface *const event;
    const ExtensionExtenderInterface * const extension;
    const LogExtenderInterface * const log;

} ExtenderInterface;

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


#ifdef __cplusplus
/************************************************************************/
/* Extender Interface For C++                                           */
/************************************************************************/

class ExtenderEx
{
    const ExtenderInterface *pi;

    void ThrowIfNotInitialized() const
    {
        if(pi == 0)
            throw std::runtime_error("ExtenderInterfaceEx has not been initialized properly");
    }

public:
    ExtenderEx():pi(0){}
    ExtenderEx(const ExtenderInterface *pi) : pi(pi) {}
    virtual ~ExtenderEx() {}

    const ExtenderInterface &GetExtenderInterface() const 
    { 
        ThrowIfNotInitialized();

        return *pi; 
    }
    const ExtenderInterface *GetExtenderInterfacePtr() const 
    { 
        ThrowIfNotInitialized();

        return pi; 
    }

    void SetExtenderInterface(const ExtenderInterface *pi)
    {
        this->pi = pi;
    }

    operator const ExtenderInterface*() const 
    {
        ThrowIfNotInitialized();

        return pi;
    }

    operator const ExtenderInterface&() const
    {
        ThrowIfNotInitialized();

        return *pi;
    }
};

class EventExtenderInterfaceEx : public virtual ExtenderEx
{
public:
    EventExtenderInterfaceEx(){}
    EventExtenderInterfaceEx(const ExtenderInterface *pi) : ExtenderEx(pi) {}
    virtual ~EventExtenderInterfaceEx(){}

    void AddEvent(const char *name)
    {
        GetExtenderInterface().event->AddEvent(name);
    }
    void FireEvent(const char *name, void *data)
    {
        GetExtenderInterface().event->FireEvent(name, data);
    }
};

class ExtensionExtenderInterfaceEx : public virtual ExtenderEx
{
public:
    ExtensionExtenderInterfaceEx(){}
    ExtensionExtenderInterfaceEx(const ExtenderInterface *pi) : ExtenderEx(pi) {}
    virtual ~ExtensionExtenderInterfaceEx(){}

    void AddExtension(const char *name, const ExtensionFactory *fac)
    {
        GetExtenderInterface().extension->AddExtension(name, fac);
    }
};

class LogExtenderInterfaceEx : public virtual ExtenderEx
{
public:
    LogExtenderInterfaceEx(){}
    LogExtenderInterfaceEx(const ExtenderInterface *pi) : ExtenderEx(pi) {}
    virtual ~LogExtenderInterfaceEx(){}

    void AddLogger(LogLevel level, LogOutputFunc output, LogFormatterFunc formatter=0, LogFilterFunc filter=0)
    {
        GetExtenderInterface().log->AddLogger(level, output, formatter, filter);
    }
    void AddLoggerAllLevel(const PluginInterface &pi, LogOutputFunc output, LogFormatterFunc formatter=0, LogFilterFunc filter=0)
    {
        GetExtenderInterface().log->AddLogger(pi.log->Debug, output, formatter, filter);
        GetExtenderInterface().log->AddLogger(pi.log->Info, output, formatter, filter);
        GetExtenderInterface().log->AddLogger(pi.log->Warn, output, formatter, filter);
        GetExtenderInterface().log->AddLogger(pi.log->Error, output, formatter, filter);
    }
};

class ExtenderInterfaceEx : public virtual InterfaceEx,
                          public EventExtenderInterfaceEx, 
                          public ExtensionExtenderInterfaceEx,
                          public LogExtenderInterfaceEx
{
public:
    ExtenderInterfaceEx(){}

    ExtenderInterfaceEx(const ExtenderInterface *pi) : 
        EventExtenderInterfaceEx(pi), 
        ExtensionExtenderInterfaceEx(pi)
        {}

    ExtenderInterfaceEx &operator=(const ExtenderInterface *pi)
    {
        this->SetExtenderInterface(pi);
        return *this;
    }
};

#endif

#endif


