#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <windows.h>

#include "extensionapi.h"
#include "console/include/console.hpp"
using namespace db;

AXF_EXTENSION_DESCRIPTION(AXF_MAKE_VERSION(1,0,0), OnInitExtension, "Win32 Console Logger", "Hunter", "Displays the AXF log on a Win32 console")

static PluginInterfaceEx pi;
static ExtenderInterfaceEx ei;
static console *con;


static void OpenConsole()
{
    con = new console(200);
    con->title(L"AXF Console by Hunter").resize(1100,800).background(console::rgb(220, 220, 220)).show();
}

static void ConsoleLogOutput(LogLevel level, const char *s)
{
    const char *title = "Unknown Error";

    /*if(level == pi.Info())
        title = "INFO";
    else if(level == pi.Debug())
        title = "DEBUG";
    else if(level == pi.Error())
        title = "ERROR";
    else if(level == pi.Warn())
        title = "WARN";*/
    
    size_t len = strlen(s)+10;
    wchar_t *buffer = (wchar_t*)alloca(len * 2);
    size_t end = mbstowcs(buffer, s, len);
    buffer[end] = L'\n';
    buffer[end+1] = 0;
    con->write(buffer, INFINITE);
}

static AxfBool OnInitExtension(const struct _PluginInterface *p, const struct _ExtenderInterface *e)
{
    pi = p;
    ei = e;

    OpenConsole();

    pi.SubscribeEvent(ON_FINALIZE_EVENT, OnFinalize);
    ei.AddLoggerAllLevel(pi, &ConsoleLogOutput, 0, 0);

    pi.Log(pi.GetAboutMessage());

    return AXFTRUE;
}

static void OnFinalize(void*)
{
    delete con;
}
