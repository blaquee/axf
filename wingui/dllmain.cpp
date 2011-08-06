#include <Windows.h>
#include "extensionapi.h"
#include "resource.h"

static PluginInterfaceEx pi;
static ExtenderInterfaceEx ei;

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

static INT_PTR __stdcall DialogCallback(HWND hwnd, UINT uMsg, WPARAM lparam, LPARAM wparam)
{
    char buffer[2000];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        UpdatePluginLists(hwnd);
        return TRUE;

    case WM_COMMAND:
        switch(lparam)
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
                            pi.Log(pi.Error, err.c_str());
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
                            pi.Log(pi.Error, err.c_str());
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

AXF_API int OnExtend(const struct _PluginInterface *p, const struct _ExtenderInterface *e)
{
    pi = p;
    ei = e;
    
    DWORD tid;
    CreateThread(0,0, &DialogThread, 0, 0, &tid);

    return AXF_PLUGIN_VERSION;
}

