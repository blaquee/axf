#include "../pch.h"
#include "../hookah.h"
#include "wsplugin.h"
#include "wsexception.h"


using namespace std;


DllPlugin::DllPlugin( const std::string &dir, const std::string &name )
: Plugin(dir, name),
OnLoad(0)
{

}

DllPlugin::~DllPlugin()
{
}


int DllPlugin::Load()
{
    string fileName = GetFilePath();

    module = std::shared_ptr<ModuleLoaderHider>(new ModuleLoaderHider(fileName));

    OnLoad = (OnLoadType) PesGetProcAddress(*module, "OnLoad");

    if(OnLoad)
    {

        try
        {
            GetPluginInterface().data->moduleHandle = (void*)*module;
            return OnLoad(&GetPluginInterface());
        }
        catch (const WSException &)
        {
            throw;
        }
    }
    else
    {
        throw WSException("this dll has not exported OnLoad");
    }
}

void DllPlugin::Unload()
{
    set<EventFunctionData*> &eventData = GetPluginInterface().data->registerEvents[ON_UNLOAD_EVENT];

    if(eventData.empty())
    {
        throw WSException("this dll has not exported OnUnload");
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


