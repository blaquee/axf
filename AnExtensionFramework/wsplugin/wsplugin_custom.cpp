#include "../pch.h"
#include "../hookah.h"
#include "wsplugin.h"
#include "wsexception.h"
#include "wspluginmanager.h"
#include "../extensionman/extension.h" // plugindata and stuff

using namespace std;

//////////////////////////////////////////////////////////////////////////
// private implementations

static const int PLUGINDATA_MAX_FILE_SIZE = 1024*1024*100; // cap it to 100MB
static WsBool PluginData_GetBinary(struct _PluginData *data, PluginBinary *outbin)
{
    std::string path = PluginManager::inst().GetPluginDirectory() + data->name;
    std::ifstream file(path, std::ios_base::binary | std::ios_base::in);
    std::ifstream::pos_type beg = file.tellg();
    file.seekg(0, std::ios_base::end);
    unsigned int result  = (unsigned int)(file.tellg() - beg);
    file.seekg(0, std::ios_base::beg);
    if(result < PLUGINDATA_MAX_FILE_SIZE && file.good())
    {
        if(outbin)
        {
            unsigned char *filebuf = new unsigned char[result+1]; //allocate 1 for null terminator
            outbin->size = result;
            outbin->data = filebuf;
            if(file.read((char*)outbin->data, outbin->size))
            {
                filebuf[result] = 0; // null terminate it
                return WSTRUE;
            }
            else
            {
                delete[] outbin->data;
                return WSFALSE;
            }
        }
    }
    return WSFALSE;
}

static void PluginData_ReleaseBinary(PluginBinary *bin)
{
    if(bin->size > 0 && bin->data)
    {
        delete[] bin->data;
    }
}


CustomPlugin::CustomPlugin( const std::string &dir, const std::string &name, const std::string &ext )
    : Plugin(dir, name), ext(ext), clientHandle(0), OnPluginUnload(0)
{

}

CustomPlugin::~CustomPlugin()
{
}

struct StopOnClientHandleValid
{
    bool operator()(const PluginData &data) const
    {
        if(data.clientHandle)
            return true;
        else
            return false;
    }
};
int CustomPlugin::Load()
{
    
    PluginData pluginData;
    pluginData.clientVersion = 0;
    pluginData.clientHandle = 0;
    pluginData.OnPluginUnload = 0;

    pluginData.extension = ext.c_str();
    pluginData.name = GetFileName().c_str();
    pluginData.GetBinary = &PluginData_GetBinary;
    pluginData.ReleaseBinary = &PluginData_ReleaseBinary;
    
    try 
    {
        PluginManager::inst().FireEventStopIf(ON_LOAD_PLUGIN, pluginData, StopOnClientHandleValid());
        if(pluginData.clientHandle)
        {
            clientHandle = pluginData.clientHandle;
            OnPluginUnload = pluginData.OnPluginUnload;
            return pluginData.clientVersion;
        }
        else
        {
            throw WSException(std::string("Custom Plugin did not specify clientHandle: ") + GetFileName());
        }
    }
    catch(const WSException &ex)
    {
        throw WSException(std::string("Failed to custom load plugin: ") + GetFileName() + std::string("\nInfo: ") + ex.what());
    }
    catch(...)
    {
        throw WSException(std::string("Failed to custom load plugin: ") + GetFileName());
    }
}

void CustomPlugin::Unload()
{
    if(OnPluginUnload == 0)
    {
        throw WSException("this custom plugin has not defined OnPluginUnload");
    }
    else
    {
        OnPluginUnload(clientHandle);
    }
}


