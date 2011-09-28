#include "extensionapi.h"
#include <ucdl.h>
#include <string>
#include <sstream>

#include "binding.h"

#pragma comment(lib, "underc.lib")

AXF_EXTENSION_DESCRIPTION(1, OnInitExtension, "UnderC Custom Plugin Loader", "Hunter", "Load UnderC with this extension")

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

static void OnUnloadPlugin(WsHandle clientHandle)
{
    // no clean up required
}

static void OnLoadPlugin(void *arg)
{
    PluginData *pluginData = (PluginData*)arg;
    PluginBinary script;
    if(pluginData->GetBinary(pluginData, &script))
    {
        AutoRelease rls(*pluginData, script);
        if(!uc_exec((char*)script.data))
        {
            char errorMsg[1024];
            uc_error(errorMsg, sizeof(errorMsg));
            errorMsg[sizeof(errorMsg)-1] = 0; // cap it just in case
            pi.RaiseException(errorMsg);
        }
        pluginData->clientVersion = AXF_API_VERSION; // required for version compatibility
        pluginData->clientHandle = (WsHandle)1; //dummy value, the plugin manager requires the clientHandle to be non null
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
    uc_include("string");
    uc_include("vector");
    uc_include("axf/pluginapi.h");

    InitBinding();

    pi.SubscribeEvent(ON_LOAD_PLUGIN, OnLoadPlugin);
}

#define BIND(ret, arg, func) uc_import(ret " " NAMESPACE #func arg, &axf::func)
void InitBinding()
{
    BIND("void*", "()", GetModuleHandle);
    BIND("ProcessInfo", "()", GetProcessInformation);
    BIND("void*", "(const std::string&)", GetModuleBase);
    BIND("void*", "(void *, const std::string &)", GetProcAddress);
    BIND("void", "(const std::string &exceptionMsg=\"Unknown Exception Raised!\", void *dataUnused=0)", RaiseException);
    BIND("std::string", "()", GetAboutMessage);
    BIND("unsigned int", "()", GetVersion);
    BIND("std::string", "()", GetBaseDirectory);
    BIND("std::string", "()", GetPluginDirectory);
    BIND("std::string", "()", GetExtensionDirectory);

    BIND("LogLevel","()",Quiet);
    BIND("LogLevel","()",Debug);
    BIND("LogLevel","()",Info);
    BIND("LogLevel","()",Warn);
    BIND("LogLevel","()",Error);
    BIND("void","(const LogLevel)",SetLogLevel);
    BIND("LogLevel","()",GetLogLevel);
    BIND("void","(const std::string &s)",Log);
    BIND("void","(const LogLevel type, const std::string &s)",Log2);


    BIND("std::vector<std::string>","()",GetUnloadedPluginList);
    BIND("std::vector<std::string>","()",GetLoadedPluginList);
    BIND("WsBool","(const std::string &fileName)",LoadPlugin);
    BIND("WsBool","(const std::string &fileName)",UnloadPlugin);
    BIND("WsBool","(const std::string &fileName)",ReloadPlugin);


    BIND("std::vector<std::string>","()",GetEventList);
    BIND("WsBool","(const std::string &eventName)",IsEventAvailable);
    BIND("WsHandle","(const std::string &eventName, EventFunction eventFunc)",SubscribeEvent);
    BIND("void","(WsHandle handle)",UnsubscribeEvent);
    BIND("WsBool","(WsHandle handle)",IsEventSubscribed);


    BIND("WsHandle","(void *oldAddress, void *newAddress)",HookFunction);
    BIND("WsBool","(WsHandle handle)",UnhookFunction);
    BIND("WsBool","(void *oldAddress)",IsHooked);
    BIND("void *","(WsHandle handle)",GetOriginalFunction);
    BIND("ProtectionMode","(void *address, size_t size, ProtectionMode newProtection)",VirtualProtect);
    BIND("void *","(const AllocationInfo *allocInfo, const char *sig)",FindSignature);
    BIND("std::vector<std::string>","()",GetExtensionList);
    BIND("WsBool","(const std::string &extName)",IsExtensionAvailable);
    BIND("WsHandle", "(const std::string &extName)", GetExtension);
    BIND("WsBool","(WsExtension ext)",ReleaseExtension);


}

