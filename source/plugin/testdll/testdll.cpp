/*

Copyright (c) 2009, Hunter and genuine (http://mp.reversing.us)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the MPReversing nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY Hunter and genuine (http://mp.reversing.us) ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Hunter and genuine (http://mp.reversing.us) BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

void VisitUs()
{
    http://mp.reversing.us
;}

#include "pluginapi.h"
#include <sstream>

AXF_PLUGIN_DESCRIPTION(AXF_MAKE_VERSION(1,0,0), OnInit, "A test plugin", "Hunter", "For testing AXF and PluginInterface")

static PluginInterfaceEx pi;

static void writeStr(const std::string &s)
{
    pi.Log(s);
}
static void writeStr(std::stringstream &ss, bool clear=true)
{
    pi.Log(ss.str());

    if(clear)
        ss.str("");
}

bool Is64BitProcess()
{
    typedef DWORD (WINAPI * IsWow64ProcessT)(HANDLE, LPDWORD);

#if defined(_WIN64)
    return true;  // 64-bit programs run only on Win64
#elif defined(_WIN32)
    IsWow64ProcessT p = (IsWow64ProcessT)GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process");
    DWORD isWow64;
    if(p && p(GetCurrentProcess(), &isWow64) && isWow64)
    {
        return false;
    }
    else if(sizeof(void*) == 8)
    {
        return true;
    }
    else
    {
        return false;
    }
#else
"only win32 or win64 target is supported"
#endif
}

static AxfHandle hookedMsgBox=0;

INT __stdcall myMsgBox(HWND hwnd, LPSTR, LPSTR, UINT)
{
    typedef INT (WINAPI *tMsgBox)(HWND,LPCSTR, LPCSTR, UINT);
    tMsgBox MB = (tMsgBox)pi.GetOriginalFunction(hookedMsgBox);
    return MB(hwnd, "You called the hooked msgbox", "It works", 0);
}

INT __stdcall MsgBoxBP(HWND hwnd, LPSTR, LPSTR, UINT)
{
    typedef INT (WINAPI *tMsgBox)(HWND,LPCSTR, LPCSTR, UINT);
    pi.Log("CALL FROM BREAKPOINT!");
    return 1;
}

void TestBp(AxfHandle threadId, AxfHandle processId)
{
    if (pi.GetCurrentProcessId() == processId)
    {
        pi.SetBreakpointFunc(threadId, 0, MessageBoxA, MsgBoxBP);
    }
}
unsigned int bptestvar = 10;
void TestBp(void *data)
{
    pi.Log("WTF");
}
AxfBool OnInit(const PluginInterface *p)
{
    pi = p;

    std::stringstream ss;

    // event subsystem testing
    ss << "Is ON_UNLOAD_EVENT Event Available " << pi.IsEventAvailable(ON_FINALIZE_EVENT); writeStr(ss);

    ss << "Is OnFakeEvent Available " << pi.IsEventAvailable("OnFakeEvent"); writeStr(ss);
    ss << "Is Bogus Event Subscribed " << pi.IsEventSubscribed((AxfHandle)1234); writeStr(ss);

    pi.SubscribeEvent(ON_FINALIZE_EVENT, OnFinalize);

    ss << "Unsubscribe to a bogus event"; writeStr(ss);
    pi.UnsubscribeEvent((AxfHandle)12345);

    // print a list of all the events available
    std::vector<std::string> eventList = pi.GetEventList();
    ss << std::endl << "Events Available:" << std::endl;
    for (size_t i = 0;i < eventList.size();i++)
    {
        ss << eventList[i] << std::endl;
    }
    ss << std::endl;
    writeStr(ss);

    //rangetest();
    //testhash();

    if(Is64BitProcess())
    {
        ss << "You are running inside 64bit process" << std::endl;
    }
    else
    {
        ss << "You are running 32bit process" << std::endl;
    }
    writeStr(ss);

    std::vector<std::string> pluginList = pi.GetUnloadedPluginList();
    ss << "Plugins Available for loading:" << std::endl;
    for (size_t i = 0;i < pluginList.size();i++)
    {
        ss << pluginList[i] << std::endl;
    }
    ss << std::endl;
    writeStr(ss);

    ss << "Module Handle: " << pi.GetModuleHandle() << std::endl; writeStr(ss);

    ProcessInfo pInfo = pi.GetProcessInformation();
    ss << "Process: " << pInfo.processName << ":" << pInfo.processID << std::endl; writeStr(ss);

    writeStr(ss);

    ss << "plugindesc Address from pointer: " << &plugindesc << std::endl;
    ss << "Using PluginInterface.GetProcAddress('plugindesc'): " << pi.GetProcAddress(pi.GetModuleHandle(), "plugindesc") << std::endl;
    writeStr(ss);

    std::vector<std::string> extensionList = pi.GetExtensionList();
    ss << "Available Extensions:" << std::endl;
    for (size_t i = 0;i < extensionList.size();i++)
    {
        ss << extensionList[i] << std::endl;
    }
    ss << std::endl;
    writeStr(ss);

    if(extensionList.empty() == false)
    {
        std::string &extName = extensionList[0];
        ss << "Is " << extName << " Available " << pi.IsExtensionAvailable(extName); writeStr(ss);
        AxfExtension realExt = pi.GetExtension(extName);
        ss << "Getting a Real Extension Returned: " << realExt; writeStr(ss);
        ss << "Destroying Real Extension Returned: " << pi.ReleaseExtension(realExt); writeStr(ss);
    }
    writeStr(ss);

    ss << "Is FakeExtension_123 Available " << pi.IsExtensionAvailable("FakeExtension_123"); writeStr(ss);
    AxfExtension fakeExt = pi.GetExtension("FakeExtension_123");
    ss << "Getting a Fake Extension Returned: " << fakeExt; writeStr(ss);
    ss << "Destroying Fake Extension Returned: " << pi.ReleaseExtension(fakeExt) << std::endl; writeStr(ss);

    //raise an exception if you dont want the plugin to load (only do this when something goes wrong)
    //pi.RaiseException("Somefing wong!");
    hookedMsgBox = pi.HookFunction((void*)&MessageBoxA, (void*)&myMsgBox);
    if(hookedMsgBox)
    {
        ss << "Hooked MessageBoxA(), handle: " << hookedMsgBox << std::endl; writeStr(ss);
        ss << "Calling MessageBoxA()... "; writeStr(ss); MessageBoxA(0, "FAILED to call hooked MsgBoxA", "Failed", 0); writeStr(ss);
        ss << "Unhooked MessageBoxA() ret:" << pi.UnhookFunction(hookedMsgBox) << std::endl; writeStr(ss);
        ss << "Calling MessageBoxA()... "; writeStr(ss); MessageBoxA(0, "You called the original MsgBoxA", "It works", 0); writeStr(ss);
    }
    else
    {
        pi.Log("Failed to hook MessageBoxA");
    }

    // test sig scanning
    std::unique_ptr<AllocationInfo> allocBase = pi.GetAllocationBase((void*)&ExitWindowsEx);
    if(allocBase)
    {
        ss << "base of ExitWindowsEx (user32.dll):"<< allocBase->base << " size:" << allocBase->size << std::endl; writeStr(ss);
        ss << "base of user32.dll:"<< pi.GetModuleBase("user32.dll") << std::endl; writeStr(ss);

        const char *exitWindowsExSig =
                                "8BFF" //mov     edi, edi
                                "55" //push    ebp
                                "8BEC" //mov     ebp, esp
                                "83EC 18" //sub     esp, 18h
                                "53" //push    ebx
                                "8B5D 08" //mov     ebx, [ebp+8]
                                "56" //push    esi
                                "8BF3" //mov     esi, ebx
                                "81E6 0B580000" //and     esi, 580Bh
                                "F7DE" //neg     esi
                                "1BF6" //sbb     esi, esi
                                "F7DE" //neg     esi
                                "57" //push    edi
                                "BF 00000200" //mov     edi, 20000h
                                "74 71" //jz      short loc_7DCB152B
                                "F6C3 04" //test    bl, 4
                                "75 6C" //jnz     short loc_7DCB152B
                                "833D ???????? 00" //cmp     dword_????????, 0
                                "74 04"; //jz      short loc_7DCB14CC

        ss << "SigScan of ExitWindowsEx: " << pi.FindSignature(allocBase.get(), exitWindowsExSig) << std::endl; writeStr(ss);
        ss << "Real address of ExitWindowsEx: " << &ExitWindowsEx << std::endl; writeStr(ss);
    }
    else
    {
        pi.Log("Failed to obtain Allocation base for ExitWindowsEx\n");
    }

    // test hardware breakpoint
    //pi.EnumerateThreads(TestBp);
    pi.SetBreakpointFunc(pi.GetCurrentThreadId(), 0, MessageBoxA, MsgBoxBP);
    MessageBoxA(0, "testing", "should not disaply", 0);
    pi.DeleteBreakpoint(pi.GetCurrentThreadId(), 0); // this shouldnt be neccessary once I add clean up code to axf

    // test hardware breakpoint for read/writing variable
    
    pi.SetBreakpointVar(pi.GetCurrentThreadId(), 0, AXFTRUE, AXFTRUE, 4, &bptestvar,
        [](void *data)
        {
            /*unsigned int v = *(unsigned int*)data;
            std::stringstream ss; ss << "Var was modified, new value: " << v;
            pi.Log(ss.str());*/
        printf("wtf\n");
        }, &bptestvar);
    bptestvar = pi.GetVersion();
    pi.DeleteBreakpoint(pi.GetCurrentThreadId(), 0); // this shouldnt be neccessary once I add clean up code to axf

    pi.Log("test plugin loaded");

    return AXFTRUE;
}

static void OnFinalize(void*)
{
    pi.Log("test plugin unloaded");

}

