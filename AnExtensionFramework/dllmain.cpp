#include "pch.h"
#include "hookah.h"
#include "wsplugin/wspluginmanager.h"
#include "json/reader.h"
#include "json/writer.h"

wchar_t REGISTRY_KEY[] = L"Software\\AXF";

static const std::string CONFIG_FILENAME = "config.axf";

//////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE handle, DWORD reason, LPVOID reserved)
{
    return EasyhookDllMain(handle, reason, reserved);
}


static void InitExtensions()
{
    const std::string CONFIG_FILEPATH = PluginManager::inst().GetExtensionDirectory() + CONFIG_FILENAME;

    std::ifstream configFile(CONFIG_FILEPATH);
    if(configFile.is_open())
    {
        try
        {
            json::Object doc;
            json::Reader::Read(doc, configFile);

            json::Array extList = doc["extensions"];

            for(json::Array::const_iterator it = extList.Begin(); it != extList.End(); ++it)
            {
                try
                {
                    json::String dllName = *it;
                    PluginManager::inst().LoadExtension(dllName);
                }
                catch(const std::exception &ex)
                {
                    MessageBoxA(0, ex.what(), "ExtInit Error", 0);
                }
            }
        }
        catch(const std::exception &ex)
        {
            MessageBoxA(0, ex.what(), "ExtInit Error", 0);
        }
        
    }
    else
    {
        std::string err = std::string("Failed to open ") + CONFIG_FILEPATH;
        MessageBoxA(0, err.c_str(), "ExtInit Error", 0);
    }
}

void Init()
{
    InitExtensions();
}

//////////////////////////////////////////////////////////////////////////


