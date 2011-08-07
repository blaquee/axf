#include "../pch.h"
#include "../hookah.h"
#include "../wsplugin/wspluginmanager.h"
#include "extension.h"

AXFExtension::AXFExtension()
{

}

AXFExtension::~AXFExtension()
{

}

namespace
{
    void AddEvent_Extender(const char *name)
    {
        PluginManager::inst().GetEvents()[name];
    }

    void FireEvent_Extender(const char *name, void *data)
    {
        auto eventDataIt = PluginManager::inst().GetEvents().find(name);
        if(eventDataIt == PluginManager::inst().GetEvents().end())
        {
            return;
        }

        if(eventDataIt->second.empty())
        {
            return;
        }

        for(std::set<EventFunctionData*>::const_iterator it = eventDataIt->second.begin(); it != eventDataIt->second.end(); ++it)
        {
            EventFunction eventFunc = (*it)->func;

            if(eventFunc)
            {
                eventFunc(data);
            }
        }
    }

    void AddExtension_Extender(const char *name, const ExtensionFactory *fac)
    {
        PluginManager::inst().GetExtensionFactories()[name] = *fac;
    }
}


void AXFExtension::SetupExtenderInterface( ExtenderInterface &extender )
{
    extender.data = new ExtenderInterfaceData;
    extender.event = new EventExtenderInterface;
    extender.extension = new ExtensionExtenderInterface;

    extender.event->AddEvent = &AddEvent_Extender;
    extender.event->FireEvent = &FireEvent_Extender;

    extender.extension->AddExtension = &AddExtension_Extender;
}

