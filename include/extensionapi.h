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
/* Export Functions/Events                                              */
/************************************************************************/

/* The following functions must be implemented with C (__cdecl) calling convention and exported from the dll */

/* called when the plugin gets loaded
 the plugin cannot be loaded if this function isn't implemented 
 
 Must return AXF_PLUGIN_VERSION
 */
AXF_API int OnExtend(const struct _PluginInterface *, const struct _ExtenderInterface *);


/************************************************************************/
/* Extender Interface                                                   */
/************************************************************************/

typedef struct _EventExtenderInterface
{
    void (*AddEvent)(const char *name);
}EventExtenderInterface;

typedef struct _ExtensionExtenderInterface
{
    void (*AddExtension)(const char *name, const struct _ExtensionFactory *fac);
}ExtensionExtenderInterface;

typedef struct _ExtenderInterface
{
    const void * const data;

    const EventExtenderInterface *const event;
    const ExtensionExtenderInterface * const extension;

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

    const ExtenderInterface &GetPluginInterface() const 
    { 
        ThrowIfNotInitialized();

        return *pi; 
    }
    const ExtenderInterface *GetPluginInterfacePtr() const 
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


};

class ExtensionExtenderInterfaceEx : public virtual ExtenderEx
{
public:
    ExtensionExtenderInterfaceEx(){}
    ExtensionExtenderInterfaceEx(const ExtenderInterface *pi) : ExtenderEx(pi) {}
    virtual ~ExtensionExtenderInterfaceEx(){}


};

class ExtenderInterfaceEx : public virtual InterfaceEx,
                          public EventExtenderInterfaceEx, 
                          public ExtensionExtenderInterfaceEx
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


