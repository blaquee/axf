#ifndef extensionmanager_h__
#define extensionmanager_h__

#include "../singleton.h"
struct ExtensionFactory;
struct ExtenderInterface;

class AXFExtension : public Singleton<AXFExtension>
{
    friend class Singleton<AXFExtension>;

    AXFExtension();
    virtual ~AXFExtension();

    AXFExtension(const AXFExtension&);
public:
    void SetupExtenderInterface(ExtenderInterface&);
};

//////////////////////////////////////////////////////////////////////////

class ExtenderInterfaceData
{
    int unused;
};

struct EventExtenderInterface
{
    void (*AddEvent)(const char *name);
    void (*FireEvent)(const char *name, void *data);
};

struct ExtensionExtenderInterface
{
    void (*AddExtension)(const char *name, const ExtensionFactory *fac);
};

struct ExtenderInterface
{
    ExtenderInterfaceData * data;

    EventExtenderInterface *event;
    ExtensionExtenderInterface *extension;
};

#endif // extensionmanager_h__

