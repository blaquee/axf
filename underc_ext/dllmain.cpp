#include "extensionapi.h"
#include <ucdl.h>
#include <string>
#include <sstream>

#include "binding.h"

#pragma comment(lib, "underc.lib")

AXF_EXTENSION_DESCRIPTION(1, OnInitExtension, "UnderC Custom Plugin Loader", "Hunter", "Load UnderC with this extension")

static const char *EMPTY_STRING="";

static PluginInterfaceEx pi=0;
static ExtenderInterfaceEx ei=0;

void InitBinding();

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

struct UnderCPlugin
{
    PluginDescription pluginDesc;
    UnderCPlugin()
    {
        pluginDesc.version = 0; 
        pluginDesc.pluginapiVersion = 0; 

        pluginDesc.OnInit = 0;

        pluginDesc.name = 0;
        pluginDesc.author = 0; 
        pluginDesc.about = 0;  

        pluginDesc.reserved0 = 0;
        pluginDesc.reserved1 = 0; 
    }
    void DumpPluginInfo()
    {
        std::stringstream ss;
        ss << "version: " << pluginDesc.version << std::endl
            << "pluginapiVersion: " << pluginDesc.pluginapiVersion << std::endl
            << "OnInit: " << pluginDesc.OnInit << std::endl
            << "name: " << pluginDesc.name << std::endl
            << "author: " << pluginDesc.author << std::endl
            << "about: " << pluginDesc.about << std::endl
            << "reserved0: " << pluginDesc.reserved0 << std::endl
            << "reserved1: " << pluginDesc.reserved1 << std::endl;
        pi.Log(ss.str());
    }
};

static void OnUnloadPlugin(WsHandle clientHandle)
{
    UnderCPlugin *ucp = (UnderCPlugin*)clientHandle;
    delete (void*)(ucp->pluginDesc.OnInit); // HACK: underc allocates it using new/delete
    delete ucp;
}

static void OnLoadPlugin(void *arg)
{
    PluginData *pluginData = (PluginData*)arg;
    PluginBinary script;
    if(pluginData->GetBinary(pluginData, &script))
    {
        AutoRelease rls(*pluginData, script);
        UnderCPlugin *ucp = new UnderCPlugin;
        uc_init_ref("PluginDescription", "__plugindesc__", &ucp->pluginDesc);
        if(!uc_exec((char*)script.data))
        {
            delete ucp;

            char errorMsg[1024];
            uc_error(errorMsg, sizeof(errorMsg)-2);
            errorMsg[sizeof(errorMsg)-1] = 0; // cap it just in case
            pi.RaiseException(errorMsg);
        }
        ucp->pluginDesc.name = ucp->pluginDesc.name ? ucp->pluginDesc.name : EMPTY_STRING;
        ucp->pluginDesc.author = ucp->pluginDesc.author ? ucp->pluginDesc.author : EMPTY_STRING;
        ucp->pluginDesc.about = ucp->pluginDesc.about ? ucp->pluginDesc.about : EMPTY_STRING;

        if(ucp->pluginDesc.pluginapiVersion > pi.GetVersion())
        {
            delete ucp;
            pi.RaiseException("AXF is too old");
        }
        if(ucp->pluginDesc.OnInit == 0)
        {
            delete ucp;
            pi.RaiseException("Custom Plugin has not exported OnInit");
        }

        // HACK: make OnInit executable, underc doesnt make the returned buffer executable!
        pi.VirtualProtect(ucp->pluginDesc.OnInit, 200, PROTECTION_MODE_EXECUTE_READWRITE);
        ucp->pluginDesc.OnInit(pi.GetPluginInterfacePtr());
        pluginData->clientVersion = ucp->pluginDesc.pluginapiVersion; // required for version compatibility
        pluginData->clientHandle = (WsHandle)ucp; //dummy value, the plugin manager requires the clientHandle to be non null
        pluginData->OnPluginUnload = OnUnloadPlugin; // make the custom plugin unloadable
    }
}

// not required since extension cant be unloaded
struct DeinitUnderC
{
    DeinitUnderC(){ }

    ~DeinitUnderC()
    { 
        uc_finis(); 
    }

} underc_;

static void OnInitExtension(const struct _PluginInterface *p, const struct _ExtenderInterface *e)
{
    pi = p;
    ei = e;

    //underc requires UC_HOME points to the directory that contains the include and lib directories
    std::ostringstream ss;
    ss << "UC_HOME=" << pi.GetPluginDirectory();
    std::string uchomevar = ss.str(); // make a copy of it
    putenv(uchomevar.c_str());
    uc_init(NULL, 0);
    uc_include("axf/pluginapi.h");

    InitBinding();

    pi.SubscribeEvent(ON_LOAD_PLUGIN, OnLoadPlugin);
}

// exported to underc as pi_FuncName()
#define BIND(ret, arg, func) uc_import(ret " " NAMESPACE #func arg, &axf::func)
void InitBinding()
{
    // system
    BIND("void*", "(PluginInterface *pi)", GetModuleHandle);
    BIND("void", "(PluginInterface *pi)", GetProcessInformation);
    BIND("void*", "(PluginInterface *pi, const char *name)", GetModuleBase);
    BIND("void*", "(PluginInterface *pi, void *base, const char *name)", GetProcAddress);
    BIND("void", "(PluginInterface *pi, const char *exceptionMsg, void *dataUnused)", RaiseException);
    BIND("const char*", "(PluginInterface *pi)", GetAboutMessage);
    BIND("unsigned int", "(PluginInterface *pi)", GetVersion);
    BIND("size_t", "(PluginInterface *pi, String *s)", GetBaseDirectory);
    BIND("size_t", "(PluginInterface *pi, String *s)", GetPluginDirectory);
    BIND("size_t", "(PluginInterface *pi, String *s)", GetExtensionDirectory);

    // log
    BIND("LogLevel","(PluginInterface *pi)",Quiet);
    BIND("LogLevel","(PluginInterface *pi)",Debug);
    BIND("LogLevel","(PluginInterface *pi)",Info);
    BIND("LogLevel","(PluginInterface *pi)",Warn);
    BIND("LogLevel","(PluginInterface *pi)",Error);
    BIND("void","(PluginInterface *pi, const LogLevel)",SetLogLevel);
    BIND("LogLevel","(PluginInterface *pi)",GetLogLevel);
    BIND("void","(PluginInterface *pi, const char *s)",Log);
    BIND("void","(PluginInterface *pi, const LogLevel type, const char *s)",Log2);

    // manager
    BIND("size_t","(PluginInterface *pi, String *strs, size_t numOfStrs)",GetUnloadedPluginList);
    BIND("size_t","(PluginInterface *pi, String *strs, size_t numOfStrs)",GetLoadedPluginList);
    BIND("WsBool","(PluginInterface *pi, const char *fileName)",LoadPlugin);
    BIND("WsBool","(PluginInterface *pi, const char *fileName)",UnloadPlugin);
    BIND("WsBool","(PluginInterface *pi, const char *fileName)",ReloadPlugin);

    // event
    BIND("size_t","(PluginInterface *pi, String *strs, size_t numOfStrs)",GetEventList);
    BIND("WsBool","(PluginInterface *pi, const char *eventName)",IsEventAvailable);
    BIND("WsHandle","(PluginInterface *pi, const char *eventName, EventFunction eventFunc)",SubscribeEvent);
    BIND("void","(PluginInterface *pi, WsHandle handle)",UnsubscribeEvent);
    BIND("WsBool","(PluginInterface *pi, WsHandle handle)",IsEventSubscribed);

    // hook
    BIND("WsHandle","(PluginInterface *pi, void *oldAddress, void *newAddress)",HookFunction);
    BIND("WsBool","(PluginInterface *pi, WsHandle handle)",UnhookFunction);
    BIND("WsBool","(PluginInterface *pi, void *oldAddress)",IsHooked);
    BIND("void *","(PluginInterface *pi, WsHandle handle)",GetOriginalFunction);

    // memory
    BIND("WsBool", "(PluginInterface *pi, AllocationInfo *allocInfo, void *addr)", GetAllocationBase);
    BIND("ProtectionMode","(PluginInterface *pi, void *address, size_t size, ProtectionMode newProtection)",VirtualProtect);
    BIND("void *","(PluginInterface *pi, const AllocationInfo *allocInfo, const char *sig)",FindSignature);
    
    // extension
    BIND("size_t","(PluginInterface *pi, String *strs, size_t numOfStrs)",GetExtensionList);
    BIND("WsBool","(PluginInterface *pi, const char *extName)",IsExtensionAvailable);
    BIND("WsHandle", "(PluginInterface *pi, const char *extName)", GetExtension);
    BIND("WsBool","(PluginInterface *pi, WsExtension ext)",ReleaseExtension);
}

