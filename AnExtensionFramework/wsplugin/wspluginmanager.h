#ifndef wspluginmanager_h__
#define wspluginmanager_h__


#include "../singleton.h"
#include "wsplugin.h"

extern wchar_t REGISTRY_KEY[]; // user defined in dllmain.cpp

class PluginManager : public Singleton<PluginManager>
{

private:
    friend class Singleton<PluginManager>;

    std::string pluginDir, baseDir, extDir;
    std::vector<Plugin*> loadedPlugins;

    std::map<std::string, std::set<EventFunctionData*> > events;
    std::map<std::string, ExtensionFactory> extensionFactories;

public:
    static CRITICAL_SECTION mutex;

public:
    
    std::string GetBaseDirectory() const;

    void SetBaseDirectory(const std::string &dir);

    std::string GetPluginDirectory() const;

    void SetPluginDirectory(const std::string &dir);

    std::string GetExtensionDirectory() const;

    void SetExtensionDirectory(const std::string &dir);

    std::vector<std::string> GetPluginList() const;

    std::vector<std::string> GetLoadedPluginList() const;

    std::vector<std::string> GetUnloadedPluginList() const;

    void LoadExtension(const std::string &fileName);

    void LoadPlugin(const std::string &fileName);

    void UnloadPlugin(const std::string &fileName);

    void ReloadPlugin(const std::string &fileName);

    void SoftReloadPlugin(const std::string &fileName);

    // event related functions
    std::map<std::string, std::set<EventFunctionData*> > &GetEvents();
    void FireEvent(const std::string &name, void *data) const;

    template<class PredFunc, class DataType>
    void FireEventStopIf( const std::string &name, const DataType &data, PredFunc func ) const
    {
        map<string, set<EventFunctionData*> >::const_iterator eventDataIt = PluginManager::inst().GetEvents().find(name);
        if(eventDataIt == PluginManager::inst().GetEvents().end())
        {
            return;
        }

        if(eventDataIt->second.empty())
        {
            return;
        }

        for(std::set<EventFunctionData*>::const_iterator it = eventDataIt->second.begin(); it != eventDataIt->second.end(); ++it)
        {
            EventFunction eventFunc = (*it)->func;

            if(eventFunc)
            {
                eventFunc((void*)&data);
            }
            if(func(data))
            {
                return;
            }
        }
    }

    // extension related functions
    std::map<std::string, ExtensionFactory> &GetExtensionFactories(); // ext_name -> factory_function

private:
    PluginManager();
    virtual ~PluginManager();
    PluginManager(const PluginManager&){}

};

#endif // wspluginmanager_h__


