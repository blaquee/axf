#include <windows.h>
#include <string>
#include <iostream>
#include <foreach.h>
#include <axf/pluginapi.h>

// 1. declare our plugin version
unsigned int VERSION = 1;

// 2. declare our entry point
namespace test { void OnInit(const PluginInterface *pi); } 

// 3. export the plugin description
AXF_PLUGIN_DESCRIPTION(VERSION, test::OnInit, "Test Plugin", "Hunter", "Testing this plugin")


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

