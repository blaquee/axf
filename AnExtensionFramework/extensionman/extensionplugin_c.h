#ifndef extensionplugin_c_h__
#define extensionplugin_c_h__

#include "../wsplugin/wsplugin.h"

typedef int (*OnExtendType)(const PluginInterface *, const ExtenderInterface *);
class ExtensionDllPlugin : public Plugin
{
    // these will be obtained from the dlls
    OnExtendType OnLoad;
    std::shared_ptr<ModuleLoaderHider> module;
    ExtenderInterface extender;

public:
    ExtensionDllPlugin( const std::string &dir, const std::string &name );
    virtual ~ExtensionDllPlugin();

protected:
    virtual int Load();
    virtual void Unload();

};


#endif // extensionplugin_c_h__

