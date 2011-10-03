#include <windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <foreach.h>
#include <axf/pluginapi.h>

using namespace std; // for convinience

// 0. Make sure everything is wrapped in a namespace because 
//    underc shares names globally (which is a very bad idea!)
namespace skeleton 
{ 
// 1. declare our plugin version, has to be greater than 0
const unsigned int VERSION = 1;

// 2. declare our entry point
WsBool OnInit(const PluginInterface *pi); 
} // namespace skeleton


// 3. export the plugin description, please use this macro outside the namespace
AXF_PLUGIN_DESCRIPTION(skeleton::VERSION, skeleton::OnInit, "Skeleton Plugin", "Your Name Here", "A dumb Plugin that does nothing")


// 4. implementation, etc
namespace skeleton
{
PluginInterfaceEx pi;

WsBool OnInit(const PluginInterface *p)
{
    pi = p; // initialize PluginInterfaceEx (the C++ version of PluginInterface)

    pi.Log("It's ready!");
    pi.Log2(pi.Warn(), "Go write something useful you slacker!");
     
    // all plugins has to return WSTRUE
    // if an error occurs during initialization, return WSFALSE instead
    return WSTRUE;
    
    // All hooks, allocated interfaces and subscribed events will be released when you unload this plugin
}

} // namespace skeleton

