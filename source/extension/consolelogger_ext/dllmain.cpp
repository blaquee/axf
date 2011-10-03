#include <io.h>
#include <fcntl.h>
#include <iostream>

#include "extensionapi.h"

AXF_EXTENSION_DESCRIPTION(AXF_MAKE_VERSION(1,0,0), OnInitExtension, "Console Logger", "Hunter", "Displays the AXF log on a console")

static PluginInterfaceEx pi;
static ExtenderInterfaceEx ei;

static void OpenConsole()
{
    int outHandle, errHandle, inHandle;
    FILE *outFile, *errFile, *inFile;
    AllocConsole();
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    coninfo.dwSize.Y = 9999;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    outHandle = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
    errHandle = _open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE),_O_TEXT);
    inHandle = _open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE),_O_TEXT );

    outFile = _fdopen(outHandle, "w" );
    errFile = _fdopen(errHandle, "w");
    inFile =  _fdopen(inHandle, "r");

    *stdout = *outFile;
    *stderr = *errFile;
    *stdin = *inFile;

    setvbuf( stdout, NULL, _IONBF, 0 );
    setvbuf( stderr, NULL, _IONBF, 0 );
    setvbuf( stdin, NULL, _IONBF, 0 );

    std::ios::sync_with_stdio();
}

static void ConsoleLogOutput(LogLevel level, const char *s)
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

    std::cout << title << ": " << s << std::endl;
}

static WsBool OnInitExtension(const struct _PluginInterface *p, const struct _ExtenderInterface *e)
{
    pi = p;
    ei = e;

    OpenConsole();

    ei.AddLoggerAllLevel(pi, &ConsoleLogOutput, 0, 0);

    pi.Log(pi.GetAboutMessage());

    return WSTRUE;
}

