#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <cstring>

#include "extensionapi.h"
#include "angelscript.h"

// application extensions
#include "angelscript/scriptbuilder.h"
#include "angelscript/contextmgr.h"
#include "angelscript/debugger.h"
#include "angelscript/serializer.h"
#include "angelscript/scripthelper.h"

// script extensions
#include "angelscript/scriptany.h"
#include "angelscript/scriptarray.h"
#include "angelscript/scriptdictionary.h"
#include "angelscript/scriptfile.h"
#include "angelscript/scripthandle.h"
#include "angelscript/scriptmath.h"
#include "angelscript/scriptmathcomplex.h"
#include "angelscript/scriptstdstring.h"

AXF_EXTENSION_DESCRIPTION(AXF_MAKE_VERSION(1,0,0), OnInitExtension, "Angelscript", "Hunter", "AXF is now scriptable thanks to Angelscript")

struct AutoRelease
{
    PluginBinary script;
    PluginData pluginData;
    AutoRelease(PluginData pluginData, PluginBinary script) : pluginData(pluginData), script(script) {}
    ~AutoRelease()
    {
        pluginData.ReleaseBinary(&script);
    }
};

struct AutoReleaseContext
{
    asIScriptContext *ctx;
    AutoReleaseContext(asIScriptContext *ctx) : ctx(ctx) {}
    ~AutoReleaseContext()
    {
        ctx->Release();
    }
};

struct ScriptContext
{
    PluginInterfaceEx *pi; // allocated in OnLoadPlugin, deallocated in OnUnloadPlugin
};

//////////////////////////////////////////////////////////////////////////

static PluginInterfaceEx pi;
static ExtenderInterfaceEx ei;
static asIScriptEngine *scriptEngine;

static asIObjectType *arrayObjectType;
static int pluginInterfaceTypeID;
static int voidPointerTypeID;

//////////////////////////////////////////////////////////////////////////

static void __cdecl AngelMessageCallback(const asSMessageInfo *msg, void *param)
{
    const char *type = "ERR ";
    if( msg->type == asMSGTYPE_WARNING ) 
        type = "WARN";
    else if( msg->type == asMSGTYPE_INFORMATION ) 
        type = "INFO";

    std::ostringstream ss;
    ss << msg->section << " (" << msg->row << ", " << msg->col << ") : " << type << " : " << msg->message;
    pi.Log(pi.Warn(), ss.str());
}

// ptr type is name given to the void* type for angelscript

// ptr type constructors
static void __cdecl ZeroPtr(void **p)
{
    *p = 0;
}
static void __cdecl UIntToPtr32(asDWORD val, void **p)
{
    *p = (void*)val;
}
static void __cdecl UIntToPtr64(asQWORD val, void **p)
{
    *p = (void*)val;
}

// ptr type implicit casts
static std::string __cdecl PtrToString(void **p)
{
    std::ostringstream ss;
    ss << *p;
    return ss.str();
}

// ptr type explicit cast
static asDWORD __cdecl PtrToUInt32(void **p)
{
    return (asDWORD)*p;
}
static asQWORD __cdecl PtrToUInt64(void **p)
{
    return (asQWORD)*p;
}

//////////////////////////////////////////////////////////////////////////

template<typename T>
static CScriptArray *VectorToScriptArray(const std::vector<T> &vec, asIObjectType *type)
{
    CScriptArray *ary = new CScriptArray(vec.size(), type);
    for (asUINT i = 0; i < ary->GetSize(); ++i)
    {
        T *p = static_cast<T*>(ary->At(i));
        *p = vec[i];
    }

    return ary;
}

static CScriptArray *PluginMan_GetUnloadedPluginList(PluginInterfaceEx *p)
{
    static asIObjectType* t = scriptEngine->GetObjectTypeById(scriptEngine->GetTypeIdByDecl("array<string>"));

    return VectorToScriptArray(p->GetUnloadedPluginList(), arrayObjectType);
}

static CScriptArray *PluginMan_GetLoadedPluginList(PluginInterfaceEx *p)
{
    static asIObjectType* t = scriptEngine->GetObjectTypeById(scriptEngine->GetTypeIdByDecl("array<string>"));

    return VectorToScriptArray(p->GetLoadedPluginList(), arrayObjectType);
}

static CScriptArray *PluginMan_GetEventList(PluginInterfaceEx *p)
{
    static asIObjectType* t = scriptEngine->GetObjectTypeById(scriptEngine->GetTypeIdByDecl("array<string>"));

    return VectorToScriptArray(p->GetEventList(), arrayObjectType);
}

static void *PluginMan_SubscribeEvent(const std::string &event, asIScriptFunction *func, PluginInterfaceEx *p)
{
    // do something with func

    if(func)
        func->Release();

    return 0;
}

//////////////////////////////////////////////////////////////////////////

static void OnUnloadPlugin(WsHandle clientHandle)
{
    asIScriptModule *mod = (asIScriptModule*)clientHandle;

    ScriptContext *context = (ScriptContext*)mod->GetUserData();
    delete context->pi;
    delete context;
    mod->SetUserData(0);

    scriptEngine->DiscardModule(mod->GetName());
}

static void OnLoadPlugin(void *arg)
{
    PluginData *pluginData = (PluginData*)arg;
    PluginBinary script;

    if(std::strcmp(pluginData->fileExtension, "as") == 0 && pluginData->GetBinary(pluginData, &script))
    {
        AutoRelease rls(*pluginData, script);

        asIScriptModule *mod = scriptEngine->GetModule(pluginData->name, asGM_ALWAYS_CREATE);
        std::string filename = pluginData->name;

        mod->AddScriptSection(filename.c_str(), 
                              (const char*)script.data, script.size);

        int r = mod->Build();
        if(r < 0)
        {
            //pi.Log(pi.Warn(), std::string(filename) + " failed to compile!");
            pluginData->initSuccess = WSFALSE;
            pluginData->clientHandle = 0;
            pluginData->OnPluginUnload = 0;
            return;
        }
        else
        {
            asIScriptContext *ctx = scriptEngine->CreateContext();
            AutoReleaseContext autoRlsCtx(ctx);

            asIScriptFunction *func = mod->GetFunctionByDecl("int main(PluginInterface @)");
            
            if(func)
            {
                // Prepare() must be called to allow the context to prepare the stack
                ctx->Prepare(func);

                // Set the function arguments
                ScriptContext *scontext = new ScriptContext;
                scontext->pi = new PluginInterfaceEx(pluginData->pluginInterface);
                mod->SetUserData(scontext); // retrieved later in OnUnloadPlugin for cleanup purpose

                ctx->SetArgObject(0, scontext->pi);
                int r = ctx->Execute();
                if( r == asEXECUTION_FINISHED )
                {
                    // The return value is only valid if the execution finished successfully
                    int axfVersion = (int)ctx->GetReturnDWord();

                    pluginData->initSuccess = WSTRUE;
                    pluginData->clientVersion = axfVersion; // required for version compatibility
                    pluginData->clientHandle = (WsHandle)mod; 
                    pluginData->OnPluginUnload = OnUnloadPlugin; // make the custom plugin unloadable
                }
                else
                {
                    pi.Log(pi.Warn(), std::string(filename) + " failed to execute");
                    pluginData->initSuccess = WSFALSE;
                    pluginData->clientHandle = 0;
                    pluginData->OnPluginUnload = 0;
                }
                
            }
            else
            {
                pi.Log(pi.Warn(), std::string(filename) + " does not contain the entry point: int main()");
                pluginData->initSuccess = WSFALSE;
                pluginData->clientHandle = 0;
                pluginData->OnPluginUnload = 0;
                return;
            }
        }
    }
}

static WsBool SetupScriptEngine(asIScriptEngine *engine)
{
    // Set the message callback to receive information on errors in human readable form.
    int r = engine->SetMessageCallback(asFUNCTION(AngelMessageCallback), 0, asCALL_CDECL);
    if(r < 0)
    {
        engine->Release();
        return WSFALSE;
    }

    // AngelScript doesn't have a built-in string type, as there is no definite standard 
    // string type for C++ applications. Every developer is free to register it's own string type.
    // The SDK do however provide a standard add-on for registering a string type, so it's not
    // necessary to implement the registration yourself if you don't want to.
    RegisterStdString(engine);
    RegisterScriptArray(engine, true);
    RegisterStdStringUtils(engine);;
    RegisterScriptAny(engine);
    RegisterScriptDictionary(engine);
    RegisterScriptHandle(engine);
    RegisterScriptMath(engine);
    RegisterScriptMathComplex(engine);

    arrayObjectType = engine->GetObjectTypeById(engine->GetTypeIdByDecl("array<string>"));

    // Register the function that we want the scripts to call 
    const char *pluginInterfaceCls = "PluginInterface";
    pluginInterfaceTypeID = engine->RegisterObjectType(pluginInterfaceCls, 0, asOBJ_REF | asOBJ_NOCOUNT); 
    if (pluginInterfaceTypeID < 0)
    {
        engine->Release();
        return WSFALSE;
    }

    const char *voidPointerCls = "ptr";
    voidPointerTypeID = engine->RegisterObjectType(voidPointerCls, sizeof(void*), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE); 
    if (voidPointerTypeID < 0)
    {
        engine->Release();
        return WSFALSE;
    }

    engine->RegisterObjectBehaviour(voidPointerCls, asBEHAVE_CONSTRUCT, "void f()", asFUNCTION( ZeroPtr ), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour(voidPointerCls, asBEHAVE_IMPLICIT_VALUE_CAST, "string f()", asFUNCTION( PtrToString ), asCALL_CDECL_OBJLAST);

    if(sizeof(void*) == sizeof(asDWORD))
    {
        engine->RegisterObjectBehaviour(voidPointerCls, asBEHAVE_CONSTRUCT, "void f(uint)", asFUNCTION( UIntToPtr32 ), asCALL_CDECL_OBJLAST);
        engine->RegisterObjectBehaviour(voidPointerCls, asBEHAVE_VALUE_CAST, "uint f()", asFUNCTION( PtrToUInt32 ), asCALL_CDECL_OBJLAST);
    }
    else if(sizeof(void*) == sizeof(asQWORD))
    {
        engine->RegisterObjectBehaviour(voidPointerCls, asBEHAVE_CONSTRUCT, "void f(uint64)", asFUNCTION( UIntToPtr64 ), asCALL_CDECL_OBJLAST);
        engine->RegisterObjectBehaviour(voidPointerCls, asBEHAVE_VALUE_CAST, "uint64 f()", asFUNCTION( PtrToUInt64 ), asCALL_CDECL_OBJLAST);
    }

    // register custom event function pointer
    engine->RegisterFuncdef("void EventFunction(ptr)");

    // system
    engine->RegisterObjectMethod(pluginInterfaceCls, "ptr GetModuleHandle()", asMETHOD(SystemInterfaceEx,GetModuleHandle), asCALL_THISCALL);
    //engine->RegisterObjectMethod(pluginInterfaceCls, "ptr GetProcessInformation()", asMETHOD(SystemInterfaceEx,GetProcessInformation), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "ptr GetModuleBase(const string &in)", asMETHOD(SystemInterfaceEx,GetModuleBase), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "ptr GetProcAddress(ptr base, const string &in)", asMETHOD(SystemInterfaceEx,GetProcAddress), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "void RaiseException(const string &in msg=\"Unknown Exception Raised!\", ptr unused=ptr(0))", asMETHOD(SystemInterfaceEx,RaiseException), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "string GetAboutMessage()", asMETHOD(SystemInterfaceEx, GetAboutMessage), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "uint GetVersion()", asMETHOD(SystemInterfaceEx, GetVersion), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "string GetBaseDirectory()", asMETHOD(SystemInterfaceEx, GetBaseDirectory), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "string GetPluginDirectory()", asMETHOD(SystemInterfaceEx, GetPluginDirectory), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "string GetExtensionDirectory()", asMETHOD(SystemInterfaceEx, GetExtensionDirectory), asCALL_THISCALL);
    
    // logging
    engine->RegisterObjectMethod(pluginInterfaceCls, "void Log(const string &in)", asMETHODPR(LoggingInterfaceEx, Log, (const std::string&) const, void), asCALL_THISCALL);
    
    // plugin manager
    engine->RegisterObjectMethod(pluginInterfaceCls, "array<string>@ GetUnloadedPluginList()", asFUNCTION(PluginMan_GetUnloadedPluginList), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(pluginInterfaceCls, "array<string>@ GetLoadedPluginList()", asFUNCTION(PluginMan_GetLoadedPluginList), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(pluginInterfaceCls, "int LoadPlugin(const string &in)", asMETHOD(PluginManagerInterfaceEx, LoadPlugin), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "int UnloadPlugin(const string &in)", asMETHOD(PluginManagerInterfaceEx, UnloadPlugin), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "int ReloadPlugin(const string &in)", asMETHOD(PluginManagerInterfaceEx, ReloadPlugin), asCALL_THISCALL);

    // event
    engine->RegisterObjectMethod(pluginInterfaceCls, "array<string>@ GetEventList()", asFUNCTION(PluginMan_GetEventList), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(pluginInterfaceCls, "int IsEventAvailable(const string &in)", asMETHOD(EventInterfaceEx, IsEventAvailable), asCALL_THISCALL);
    engine->RegisterObjectMethod(pluginInterfaceCls, "ptr SubscribeEvent(const string &in, EventFunction @)", asFUNCTION(PluginMan_SubscribeEvent), asCALL_CDECL_OBJLAST);

    // listen for plugin load events with AXF
    pi.SubscribeEvent(ON_LOAD_PLUGIN, OnLoadPlugin);

    return WSTRUE;
}

static WsBool OnInitExtension(const struct _PluginInterface *p, const struct _ExtenderInterface *e)
{
    pi = p;
    ei = e;

    // Create the script engine
    scriptEngine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

    if(!scriptEngine)
    {
        return WSFALSE;
    }

    if(SetupScriptEngine(scriptEngine))
    {
        std::ostringstream ss;
        ss << "Loaded Angelscript Engine version: " << asGetLibraryVersion();
        pi.Log(pi.Info(), ss.str());

        return WSTRUE;
    }
    else
    {
        return WSFALSE;
    }
}

