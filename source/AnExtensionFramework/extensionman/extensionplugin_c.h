#ifndef extensionplugin_c_h__
#define extensionplugin_c_h__

#include "../wsplugin/wsplugin.h"


class ExtensionDllPlugin : public Plugin
{
    // these will be obtained from the dlls
    ExtensionDescription *extDesc;
    std::shared_ptr<ModuleLoaderHider> module;
    ExtenderInterface extender;

public:
    ExtensionDllPlugin( const std::string &dir, const std::string &name );
    virtual ~ExtensionDllPlugin();

protected:
    virtual int Load();
    virtual void Unload();
    virtual WsBool OnInit();
};


#endif // extensionplugin_c_h__

