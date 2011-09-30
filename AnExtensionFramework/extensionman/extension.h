#ifndef extensionmanager_h__
#define extensionmanager_h__

#include "../singleton.h"
struct ExtensionFactory;
struct String;

class AXFExtension : public Singleton<AXFExtension>
{
    friend class Singleton<AXFExtension>;

    AXFExtension();
    virtual ~AXFExtension();

    AXFExtension(const AXFExtension&);
public:
    void SetupExtenderInterface(struct _ExtenderInterface&);
};


//////////////////////////////////////////////////////////////////////////
// ExtensionDescription

typedef struct _ExtensionDescription
{
    unsigned int version;  /* the version of this plugin */
    unsigned int pluginapiVersion; /* the version of the pluginapi this plugin is using, must be AXF_API_VERSION (was AXF_PLUGIN_VERSION) */

    void (*OnInit)(const struct _PluginInterface*, const struct _ExtenderInterface*);  /* entry point, cdecl only */

    /* optional info */ 
    const char *name;
    const char *author; 
    const char *about;  

    void *reserved0; /* must be NULL */
    void *reserved1; /* must be NULL */
} ExtensionDescription;


//////////////////////////////////////////////////////////////////////////
// Events

// extension events
#define ON_LOAD_PLUGIN "OnLoadPlugin" // for loading of custom plugin types 

//////////////////////////////////////////////////////////////////////////

/* Used for the ON_LOAD_PLUGIN event */
typedef struct _PluginBinary
{
    const unsigned char *data;
    unsigned int size;
} PluginBinary;
typedef struct _PluginData
{
    int clientVersion; /* the extension sets this to AXF_PLUGIN_VERSION */
    WsHandle clientHandle; /* the extension sets this to non-null to signify a successful load */
    void(*OnPluginUnload)(WsHandle); /* the unload routine set by the extension, the WsHandle argument is the clientHandle */

    const char *name;
    const char *extension;
    WsBool (*GetBinary)(struct _PluginData*, PluginBinary*);
    void (*ReleaseBinary)(PluginBinary*);
} PluginData;

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

typedef struct _ExtenderInterface
{
    ExtenderInterfaceData * data;

    EventExtenderInterface *event;
    ExtensionExtenderInterface *extension;
    LogExtenderInterface *log;
} ExtenderInterface;



#endif // extensionmanager_h__

