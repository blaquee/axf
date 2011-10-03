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

    extDesc = (ExtensionDescription*) PesGetProcAddress(*module, "extdesc");

    if(extDesc)
        extDesc = *(ExtensionDescription**) extDesc; // dereference it again since its an exported variable

    if(extDesc)
    {
        if(extDesc->OnInit == 0)
        {
            throw WSException("this extension has an invalid OnInit function");
        }

        AXFExtension::inst().SetupExtenderInterface(extender);
        GetPluginInterface().data->moduleHandle = (void*)*module;
        return extDesc->pluginapiVersion;
    }
    else
    {
        throw WSException("this extension has not exported extdesc");
    }
}
WsBool ExtensionDllPlugin::OnInit()
{
    if(extDesc->OnInit)
        return extDesc->OnInit(&GetPluginInterface(), &extender);
    else
        return WSFALSE;
}
void ExtensionDllPlugin::Unload()
{
    throw WSException("Cannot unload Extensions");
}

ExtensionDllPlugin::ExtensionDllPlugin( const std::string &dir, const std::string &name )
    : Plugin(dir, name), extDesc(0)
{

}

ExtensionDllPlugin::~ExtensionDllPlugin()
{

}
