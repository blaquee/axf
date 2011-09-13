#include <Windows.h>
#include <CommCtrl.h>
#include "extensionapi.h"
#include "resource.h"

static PluginInterfaceEx pi;
static ExtenderInterfaceEx ei;

inline const char *LogLevelToText(LogLevel level)
{
    const char *text;

    if(level == pi.Info())
        text = "INFO";
    else if(level == pi.Debug())
        text = "DEBUG";
    else if(level == pi.Error())
        text = "ERROR";
    else if(level == pi.Warn())
        text = "WARN";
    else if(level == pi.Quiet())
        text = "QUIET";
    else
        text = "FATAL"; //highest

    return text;
}

static void UpdateLogLevel(HWND hwndDlg)
{
    HWND hwndLogLevelSlider = GetDlgItem(hwndDlg, IDC_LOGLEVEL);
    SendMessageA(hwndLogLevelSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pi.GetLogLevel());

    SetDlgItemTextA(hwndDlg, IDC_LOGLEVELLABEL, LogLevelToText(pi.GetLogLevel()));
}
static void SetLogLevel(HWND hwndDlg)
{
    HWND hwndLogLevelSlider = GetDlgItem(hwndDlg, IDC_LOGLEVEL);
    LRESULT level = SendMessageA(hwndLogLevelSlider, TBM_GETPOS, 0, 0);

    pi.SetLogLevel((LogLevel)level);
}

static void SetupLogLevelSlider(HWND hwndDlg)
{
    HWND hwndLogLevelSlider = GetDlgItem(hwndDlg, IDC_LOGLEVEL);
    SendMessageA(hwndLogLevelSlider, TBM_SETRANGE, (WPARAM)TRUE, MAKELONG(0, 4));
}

static void UpdatePluginLists(HWND hwndDlg)
{
    HWND hwndLoadedList = GetDlgItem(hwndDlg, IDC_LOADEDLIST);
    SendMessageA(hwndLoadedList, LB_RESETCONTENT, 0, 0);
    std::vector<std::string> plugins = pi.GetLoadedPluginList();
    for (std::vector<std::string>::const_iterator it = plugins.begin(); it != plugins.end(); ++it)
    {
        SendMessageA(hwndLoadedList, LB_ADDSTRING, 0, (LPARAM)it->c_str());
    }

    HWND hwndUnloadedList = GetDlgItem(hwndDlg, IDC_UNLOADEDLIST);
    SendMessageA(hwndUnloadedList, LB_RESETCONTENT, 0, 0);
    plugins = pi.GetUnloadedPluginList();
    for (std::vector<std::string>::const_iterator it = plugins.begin(); it != plugins.end(); ++it)
    {
        SendMessageA(hwndUnloadedList, LB_ADDSTRING, 0, (LPARAM)it->c_str());
    }
}

static INT_PTR __stdcall DialogCallback(HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
    char buffer[2000];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        SetupLogLevelSlider(hwnd);
        UpdatePluginLists(hwnd);
        UpdateLogLevel(hwnd);
        UpdateLogLevel(hwnd);
        return TRUE;

    case WM_HSCROLL:
        if(lparam == (LPARAM)GetDlgItem(hwnd, IDC_LOGLEVEL))
        {
            SetLogLevel(hwnd);
            UpdateLogLevel(hwnd);
        }
        break;

    case WM_COMMAND:
        switch(LOWORD(wparam))
        {
        case IDC_LOADBUTTON:
            {
                HWND hwndUnloadedList = GetDlgItem(hwnd, IDC_UNLOADEDLIST);
                LRESULT sel = SendMessageA(hwndUnloadedList, LB_GETCURSEL, 0,0);

                if(sel != LB_ERR)
                {
                    LRESULT len = SendMessageA(hwndUnloadedList, LB_GETTEXT, sel, (LPARAM)buffer);

                    if(len != LB_ERR && len > 0)
                    {
                        if(pi.LoadPlugin(buffer) == WSFALSE)
                        {
                            std::string err = "Can't load plugin "; err+=buffer;
                            pi.Log(pi.Error(), err.c_str());
                        }
                        else
                            UpdatePluginLists(hwnd);
                    }
                }
            }
            return TRUE;
        case IDC_UNLOADBUTTON:
            {
                HWND hwndLoadedList = GetDlgItem(hwnd, IDC_LOADEDLIST);
                LRESULT sel = SendMessageA(hwndLoadedList, LB_GETCURSEL, 0,0);

                if(sel != LB_ERR)
                {
                    LRESULT len = SendMessageA(hwndLoadedList, LB_GETTEXT, sel, (LPARAM)buffer);

                    if(len != LB_ERR && len > 0)
                    {
                        if(pi.UnloadPlugin(buffer) == WSFALSE)
                        {
                            std::string err = "Can't unload plugin "; err+=buffer;
                            pi.Log(pi.Error(), err.c_str());
                        }
                        else
                            UpdatePluginLists(hwnd);
                    }
                }
            }
            return TRUE;
        case  IDC_REFRESHBUTTON:
            UpdatePluginLists(hwnd);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static DWORD __stdcall DialogThread(LPVOID data)
{
    DialogBox((HINSTANCE)pi.GetModuleHandle(), MAKEINTRESOURCE(IDD_PLUGINDLG), 0, &DialogCallback);
    return 0;
}

static void MsgBoxLogOutput(LogLevel level, const char *s)
{
    MessageBoxA(0,s,LogLevelToText(level),0);
}

AXF_API int OnExtend(const struct _PluginInterface *p, const struct _ExtenderInterface *e)
{
    pi = p;
    ei = e;

    ei.AddLogger(pi.Error(), MsgBoxLogOutput,0,0);
    ei.AddLogger(pi.Warn(), MsgBoxLogOutput,0,0);
    
    DWORD tid;
    CreateThread(0,0, &DialogThread, 0, 0, &tid);

    return AXF_PLUGIN_VERSION;
}

