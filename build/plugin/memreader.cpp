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
namespace memreader 
{ 
// 1. declare our plugin version, has to be greater than 0, see pluginapi.h for more info about the versioning scheme
const unsigned int VERSION = AXF_MAKE_VERSION(1,0,0);

// 2. declare our entry point
WsBool OnInit(const PluginInterface *pi); 
} // namespace skeleton


// 3. export the plugin description, please use this macro outside the namespace
AXF_PLUGIN_DESCRIPTION(memreader::VERSION, memreader::OnInit, "Skeleton Plugin", "Your Name Here", "A dumb Plugin that does nothing")



// 4. implementation, etc
namespace memreader
{



template<class T>
class PtrArray 
{
public:
    char *ptr;
    unsigned int size;

    PtrArray(T *newptr, unsigned int size)
        : ptr((char*)newptr), size(size)
    {
		cout << "done" << endl;
    }

public:
    unsigned int GetSize() const
    {
        return size;
    }

    T operator[] (int i)
    {
        return *(T*)(ptr + sizeof(T)*i);
    }
};

PluginInterfaceEx pi;

WsBool OnInit(const PluginInterface *p)
{
    pi = p; // initialize PluginInterfaceEx (the C++ version of PluginInterface)

	char *obj = (char*)0x25AD4C0;
	char *hp = (char*)(obj+28);
	int *intel = (int*)(obj+29);
	int *speed = (int*)(obj+77);
	char *str = (char*)(obj+81);
	float &pos1 = *(float*)(obj+65);
	float &pos2 = *(float*)(obj+65+4);
	float &pos3 = *(float*)(obj+65+8);
	char *name = obj + 33+4;
	ostringstream ss;
	ss << (int)*hp << " " << *intel << " " << *speed << " " << (int)*str << " " 
	<< pos1 << "," << pos2 << "," << pos3 << " -- " << name << endl;
	
	
    pi.Log(ss.str());
     
    // all plugins has to return WSTRUE
    // if an error occurs during initialization, return WSFALSE instead
    return WSTRUE;
    
    // All hooks, allocated interfaces and subscribed events will be released when you unload this plugin
}

} // namespace skeleton

