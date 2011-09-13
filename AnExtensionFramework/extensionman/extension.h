#ifndef extensionmanager_h__
#define extensionmanager_h__

#include "../singleton.h"
struct ExtensionFactory;
struct ExtenderInterface;
struct String;

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

typedef void (*LogOutputFunc)(LogLevel, const char *);

typedef void (*LogFormatterFunc)(String **, LogLevel, const char *);

typedef WsBool (*LogFilterFunc)(LogLevel, const char *);

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

struct LogExtenderInterface
{
    void (*AddLogger)(LogLevel, LogOutputFunc, LogFormatterFunc, LogFilterFunc);
};

struct ExtenderInterface
{
    ExtenderInterfaceData * data;

    EventExtenderInterface *event;
    ExtensionExtenderInterface *extension;
    LogExtenderInterface *log;
};

#endif // extensionmanager_h__

