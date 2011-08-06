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
    void AddExtension_Extender(const char *name, const ExtensionFactory *fac)
    {
        PluginManager::inst().GetExtensionFactories()[name] = *fac;
    }
}


void AXFExtension::SetupExtenderInterface( ExtenderInterface &extender )
{
    extender.event = new EventExtenderInterface;
    extender.extension = new ExtensionExtenderInterface;

    extender.event->AddEvent = AddEvent_Extender;
    extender.extension->AddExtension = AddExtension_Extender;
}

