#include <io.h>
#include <fcntl.h>
#include <fstream>

#include "extensionapi.h"

AXF_EXTENSION_DESCRIPTION(AXF_MAKE_VERSION(1,0,0), OnInitExtension, "File Logger", "Hunter", "Log to console")

static PluginInterfaceEx pi;
static ExtenderInterfaceEx ei;

static std::ofstream fout;

static void FileLogOutput(LogLevel level, const char *s)
{
    const char *title = "Unknown Error";

    if(level == pi.Info())
        title = "INFO";
    else if(level == pi.Debug())
        title = "DEBUG";
    else if(level == pi.Error())
        title = "ERROR";
    else if(level == pi.Warn())
        title = "WARN";

    if(fout.good())
    {
        fout << title << ": " << s << std::endl;
        fout.flush();
    }
}

static AxfBool OnInitExtension(const struct _PluginInterface *p, const struct _ExtenderInterface *e)
{
    pi = p;
    ei = e;

    fout = std::ofstream(pi.GetBaseDirectory() + "\\axflog.txt");

    ei.AddLoggerAllLevel(pi, &FileLogOutput, 0, 0);

    pi.Log(pi.GetAboutMessage());

    return AXFTRUE;
}

