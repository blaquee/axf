#include <windows.h>
#include <string>
#include <iostream>
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

void OnInit(const PluginInterface *p)
{
    pi = p;
    
    std::string s = "It works!";
    FOREACH_AUTO(c, s)
    {
        std::cout << c << std::endl;
    }
    std::cout << "PluginInterface*: " << pi << std::endl;
    std::cout << pi.GetAboutMessage() << std::endl;
}

}

