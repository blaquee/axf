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
const unsigned int VERSION = AXF_MAKE_VERSION(1,0,0);

// 2. declare our entry point
WsBool OnInit(const PluginInterface *pi); 

} // namespace test

// 3. export the plugin description
AXF_PLUGIN_DESCRIPTION(test::VERSION, test::OnInit, "Test Plugin", "Hunter", "This plugin is used for testing AXF")


// 4. implementation, etc
namespace test 
{
PluginInterfaceEx pi;

// for some odd reason templating this function will cause a crash when this plugin is loaded twice
inline void PrintList(const char *title, std::list<std::string> l)
{
    pi.Log(title);
    FOREACH_AUTO(itm, l)
        pi.Log(itm);
    pi.Log("\n");
}

// our hook handle, you may use it to get the original function or unhook the function
WsHandle mbHook=0;

// MessageBoxA requires __stdcall convention
__stdcall int MyMsgBox(HWND hwnd, char *text, char *caption, int type)
{
    pi.Log("you have hook MessageBoxA, but unfortunately underc->c call isnt available for now");
    return 0;
}

WsBool OnInit(const PluginInterface *p)
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
    
    void *mba = pi.GetProcAddress(pi.GetModuleBase("user32.dll"), "MessageBoxA");
    mbHook = pi.HookFunction(mba, _native_stub(&MyMsgBox)); // all underc functions must be wrapped with _native_stub
    if(pi.IsHooked(mba))
        pi.Log("Hooked MessageBoxA, try clicking on the about button");
    else
        pi.Log("Failed to hook MessageBoxA");
     
    return WSTRUE;
    
    // All hooks, allocated interfaces and subscribed events will be released when you unload this plugin
}

}

