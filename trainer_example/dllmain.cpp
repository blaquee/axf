#include <sstream>
#include "pluginapi.h"
#include "ext_memoryinterface.h"

static PluginInterfaceEx pi;


/*
    This trainer plugin example uses the MemoryInterface1 Extension
    
    Coded By Hunter (2011)
*/

// Step 1. Find the address of the health point. In this case its &HEALTH.
static int HEALTH = 20; //we hack this up to 1000 using the memory interface extension

AXF_API int OnLoad(const struct _PluginInterface *p)
{
    pi = p;
    pi.SubscribeEvent(ON_FINALIZE_EVENT, &OnUnload);

    // The important bits are below this line
    //////////////////////////////////////////////////////////////////////////
    if(pi.IsExtensionAvailable(MEMORY_INTERFACE_1) == WSFALSE)
    {
        pi.RaiseException(std::string("Extension is unavailable: ") + std::string(MEMORY_INTERFACE_1));
    }
    
    MemoryInterfaceEx1 mem = pi.GetExtension<MemoryInterface1>(MEMORY_INTERFACE_1);

    int originalHealth;
    int newHealth = 1000;

    if(mem.Read(&HEALTH, &originalHealth) > 0)
    {
        std::stringstream ss; ss << "Original Health is: " << originalHealth << std::endl;
        pi.Log(ss.str());
    }
    else
    {
        pi.RaiseException("Failed to read the original health value");
    }

    if(mem.Write(&HEALTH, &newHealth) > 0)
    {
        std::stringstream ss; ss << "Modified Health is: " << HEALTH << std::endl;
        pi.Log(ss.str());
    }
    else
    {
        pi.RaiseException("Failed to write the new health value");
    }

    pi.Log("You have successfully made your first trainer!");
    //////////////////////////////////////////////////////////////////////////

    // releasing the extension is optional, the plugin manager will take care of cleaning up
    // but its a good practice to do so
    pi.ReleaseExtension(mem); 

    return AXF_PLUGIN_VERSION;
}


static void OnUnload(void *unused)
{
    // releasing the extension is optional, the plugin manager will take care of cleaning up
    //pi.ReleaseExtension(mem);
}

