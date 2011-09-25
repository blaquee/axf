#include "extensionapi.h"
#include <ucdl.h>
#include <string>
#include <sstream>

#pragma comment(lib, "underc.lib")

AXF_EXTENSION_DESCRIPTION(1, OnInitExtension, "UnderC Custom Plugin Loader", "Hunter", "Load UnderC with this extension")

static PluginInterfaceEx pi=0;
static ExtenderInterfaceEx ei=0;

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

    pi.SubscribeEvent(ON_LOAD_PLUGIN, OnLoadPlugin);
}
