#include "../pch.h"
#include "../hookah.h"
#include "../wsplugin/wspluginmanager.h"
#include "extension.h"
#include "extensionplugin_c.h"

namespace 
{
    template<typename T>
    struct ReleasableArray
    {
        T *p;
        ReleasableArray(T *p): p(p){}
        ~ReleasableArray(){ delete[] p; }
        inline operator T*() const
        {
            return p;
        }
        inline T *get() const
        {
            return p;
        }
    };
}

int ExtensionDllPlugin::Load()
{
    std::string fileName = GetFilePath();

    module = std::shared_ptr<ModuleLoaderHider>(new ModuleLoaderHider(fileName));

    OnLoad = (OnExtendType) PesGetProcAddress(*module, "OnExtend");

    if(OnLoad)
    {

        try
        {
            AXFExtension::inst().SetupExtenderInterface(extender);
            GetPluginInterface().data->moduleHandle = (void*)*module;
            return OnLoad(&GetPluginInterface(), &extender);
        }
        catch (const WSException &)
        {
            throw;
        }
    }
    else
    {
        throw WSException("this dll has not exported OnExtend");
    }
}

void ExtensionDllPlugin::Unload()
{
    throw WSException("Cannot unload Extensions");
}

ExtensionDllPlugin::ExtensionDllPlugin( const std::string &dir, const std::string &name )
    : Plugin(dir, name), OnLoad(0)
{

}

ExtensionDllPlugin::~ExtensionDllPlugin()
{

}
