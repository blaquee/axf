#include "../pch.h"
#include "../hookah.h"
#include "wsplugin.h"
#include "wsexception.h"


using namespace std;


DllPlugin::DllPlugin( const std::string &dir, const std::string &name )
: Plugin(dir, name),
pluginDesc(0)
{

}

DllPlugin::~DllPlugin()
{
}


int DllPlugin::Load()
{
    string fileName = GetFilePath();

    module = std::shared_ptr<ModuleLoaderHider>(new ModuleLoaderHider(fileName));

    pluginDesc = (PluginDescription*) PesGetProcAddress(*module, "plugindesc");

    if(pluginDesc)
        pluginDesc = *(PluginDescription**) pluginDesc; // dereference it again since its an exported variable

    if(pluginDesc)
    {
        if(pluginDesc->OnInit == 0)
        {
            throw WSException("this plugin has an invalid OnInit function");
        }
        GetPluginInterface().data->moduleHandle = (void*)*module;
        return pluginDesc->pluginapiVersion;
    }
    else
    {
        throw WSException("this plugin has not exported plugindesc");
    }
}

void DllPlugin::OnInit()
{
    if(pluginDesc && pluginDesc->OnInit)
    {
        pluginDesc->OnInit(&GetPluginInterface());
    }
}

void DllPlugin::Unload()
{
    set<EventFunctionData*> &eventData = GetPluginInterface().data->registerEvents[ON_FINALIZE_EVENT];

    if(eventData.empty())
    {
        throw WSException("this dll has not subscribed to ON_FINALIZE_EVENT");
    }

    for(set<EventFunctionData*>::const_iterator it = eventData.begin(); it != eventData.end(); ++it)
    {
        EventFunction OnUnload = (*it)->func;

        if(OnUnload)
        {
            OnUnload(0);
        }
    }
}


