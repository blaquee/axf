#include <windows.h>
#include <string>
#include <sstream>
#include <foreach.h>
#include <axf/pluginapi.h>

// 0. Make sure everything is wrapped in a namespace because 
//    underc shares names globally (which is a very bad idea!)
namespace test 
{ 
// 1. declare our plugin version
const unsigned int VERSION = 1;

// 2. declare our entry point
void OnInit(const PluginInterface *pi); 

} // namespace test

// 3. export the plugin description
AXF_PLUGIN_DESCRIPTION(test::VERSION, test::OnInit, "Test Plugin", "Hunter", "This plugin is used for testing AXF")


// 4. implementation, etc
namespace test 
{
PluginInterfaceEx pi;

template<class T>
inline void PrintList(const char *title, const T l)
{
    pi.Log(title);
    FOREACH_AUTO(itm, l)
        pi.Log(itm);
    pi.Log("\n");
}

void OnInit(const PluginInterface *p)
{
    pi = p;
    
    std::string s = "It works!";
    std::ostringstream ss; ss << "\n";
    FOREACH_AUTO(c, s)
    {
        ss << c << "\n";
    }
    pi.Log(ss.str());
    
    std::ostringstream ss2;
    ss2 << "PluginInterface*: " << pi << std::endl;
    ss2 << pi.GetAboutMessage() << std::endl;
    pi.Log(ss2.str());
    
    PrintList("Unloaded List:", pi.GetUnloadedPluginList());
    PrintList("Loaded List:", pi.GetLoadedPluginList());
    PrintList("Event List:", pi.GetEventList());
    PrintList("Extension List:", pi.GetExtensionList());
}

}

