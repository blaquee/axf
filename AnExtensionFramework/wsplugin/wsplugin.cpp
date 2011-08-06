#include "../pch.h"
#include "../hookah.h"
#include "wsplugin.h"
#include "wspluginmanager.h"
#include "wsexception.h"
#include "wsversion.h"
#include "wsthread.h"
#include "wslog.h"
#include <iterator>

using namespace std;

namespace
{

template<class StrType>
StrType clean (const StrType &oldStr, const StrType &bad) 
{
    typename StrType::iterator s, d;
    StrType str = oldStr;

    for (s = str.begin(), d = s; s != str.end(); ++ s)
    {
        if (bad.find(*s) == StrType::npos)
        {
            *(d++) = *s;
        }
    }
    str.resize(d - str.begin());

    return str;
}

size_t GetUnloadedPluginList_Plugin(String *strs, size_t sizeofStrs)
{
    vector <string> s = PluginManager::inst().GetUnloadedPluginList();
    if(sizeofStrs == 0 || strs == NULL)
    {
        return s.size();
    }
    if(sizeofStrs < s.size())
    {
        return s.size();
    }
    vector <string>::iterator it = s.begin();
    size_t i = 0;
    for( i, it ; it != s.end() || i < sizeofStrs ; ++it, i++)
    {

        StringCchCopyA(strs[i].buffer, strs[i].len, it->c_str());
        //printf("%s : length = %d\n",strs[i].buffer, strs[i].len);
    }
    return i;
}

size_t GetLoadedPluginList_Plugin(String *strs, size_t sizeofStrs)
{
    vector <string> s = PluginManager::inst().GetLoadedPluginList();
    if(sizeofStrs == 0 || strs == NULL)
    {
        return s.size();
    }
    if(sizeofStrs < s.size())
    {
        return s.size();
    }
    vector <string>::iterator it = s.begin();
    size_t i = 0;
    for( i, it ; it != s.end() || i < sizeofStrs ; ++it, i++)
    {

        StringCchCopyA(strs[i].buffer, strs[i].len, it->c_str());
        //printf("%s : length = %d\n",strs[i].buffer, strs[i].len);
    }
    return i;
}

WsBool LoadPlugin_Plugin(const char* fileName)
{
    try
    {
        PluginManager::inst().LoadPlugin(fileName);
        return WSTRUE;
    }
    catch(...)
    {
	    return WSFALSE;
    }
}

WsBool UnloadPlugin_Plugin(const char *fileName)
{
    try
    {
        PluginManager::inst().UnloadPlugin(fileName);
        return WSTRUE;
    }
    catch(...)
    {
        return WSFALSE;
    }
}

WsBool ReloadPlugin_Plugin(const char *fileName)
{
    try
    {
        PluginManager::inst().ReloadPlugin(fileName);
        return WSTRUE;
    }
    catch(...)
    {
        return WSFALSE;
    }
}
void RaiseException_Plugin(const char *msg, void *dataUnused)
{
    throw WSException(msg);
}

void *GetModuleHandle_Plugin(const PluginInterface *pi)
{
    return pi->data->moduleHandle;
}

void *GetModuleBase_Plugin(const char *name)
{
    return GetModuleHandleA(name);
}

void GetProcessInformation_Plugin(ProcessInfo *info)
{
    info->processID = GetProcessId(GetCurrentProcess());
    GetModuleFileNameA(0, (char*)info->processName, sizeof(info->processName));
    ((char*)info->processName)[sizeof(info->processName)-1] = 0;
}

void* GetProcAddress_Plugin(void *base, const char *name)
{
    return PesGetProcAddress((HMODULE)base, name);
}

size_t GetEventList_Plugin(String *strs, size_t sizeofStrs)
{
    Lock lock(&PluginManager::mutex);
    map<string, set<EventFunctionData*> > events = PluginManager::inst().GetEvents();
    lock.Unlock();

    if(sizeofStrs == 0 || strs == NULL)
    {
        return events.size();
    }
    if(sizeofStrs < events.size())
    {
        return events.size();
    }
    map<string, set<EventFunctionData*> >::const_iterator it = events.begin();
    size_t i = 0;
    for( i, it ; it != events.end() || i < sizeofStrs ; ++it, i++)
    {

        StringCchCopyA(strs[i].buffer, strs[i].len, it->first.c_str());
        //printf("%s : length = %d\n",strs[i].buffer, strs[i].len);
    }
    return i;
}

WsBool IsEventAvailable_Plugin(const char *eventName)
{
    Lock lock(&PluginManager::mutex);
    return (PluginManager::inst().GetEvents().find(eventName) != PluginManager::inst().GetEvents().end() ? WSTRUE : WSFALSE);
}

/* all event functions are __cdecl call convention */
/* returns NULL handle on failure */
WsHandle SubscribeEvent_Plugin(const struct _PluginInterface *pi, const char *eventName, EventFunction eventFunc)
{
    Lock lock(&PluginManager::mutex);
    map<string, set<EventFunctionData*> >::iterator ev = PluginManager::inst().GetEvents().find(eventName);
    
    if(ev != PluginManager::inst().GetEvents().end())
    {
        EventFunctionData *e = new EventFunctionData(pi, eventName, eventFunc);
        pi->data->registerEvents[eventName].insert(e);
        pi->data->registerEventsCache.insert(e);
        ev->second.insert(e);
        return (WsHandle)e;
    }
    else
    {
        return 0;
    }
}

/* You do not have to manually remove the event in OnUnload(), 
    the plugin manager will take care of cleaning up events */
void UnsubscribeEvent_Plugin(const struct _PluginInterface *pi, WsHandle handle)
{
    if(pi->event->IsEventSubscribed(pi, handle) == WSFALSE)
        return;

    Lock lock(&PluginManager::mutex);
    EventFunctionData *data = (EventFunctionData*)handle;
    map<string, set<EventFunctionData*> >::iterator ev = PluginManager::inst().GetEvents().find(data->name);

    if(ev != PluginManager::inst().GetEvents().end())
    {
        set<EventFunctionData*>::iterator evData = ev->second.find(data);
        if(evData != ev->second.end())
        {
            pi->data->registerEvents[data->name].erase(*evData);
            pi->data->registerEventsCache.erase(*evData);

            delete *evData;
            ev->second.erase(evData);
        }
        
    }

}

WsBool IsEventSubscribed_Plugin(const struct _PluginInterface *pi, WsHandle handle)
{
    return (pi->data->registerEventsCache.find((EventFunctionData*)handle) != pi->data->registerEventsCache.end() ? WSTRUE : WSFALSE);
}

void SetLogLevel_Plugin(const struct _PluginInterface *pi, const LogLevel type)
{
    pi->data->log.SetLogLevel((unsigned int)type);
}
LogLevel GetLogLevel_Plugin(const struct _PluginInterface *pi)
{
    return (LogLevel)pi->data->log.GetLogLevel();
}
void Log_Plugin(const struct _PluginInterface *pi, const char *s)
{
    pi->data->log.Log(s);
}
void Log2_Plugin(const struct _PluginInterface *pi, const LogLevel type, const char *s)
{
    pi->data->log.Log((unsigned int)type, s);
}

size_t GetExtensionList_Plugin(String *strs, size_t sizeofStrs)
{
    Lock lock(&PluginManager::mutex);
    map<string, ExtensionFactory> ext = PluginManager::inst().GetExtensionFactories();
    lock.Unlock();

    if(sizeofStrs == 0 || strs == NULL)
    {
        return ext.size();
    }
    if(sizeofStrs < ext.size())
    {
        return ext.size();
    }
    map<string, ExtensionFactory>::const_iterator it = ext.begin();
    size_t i = 0;
    for( i, it ; it != ext.end() || i < sizeofStrs ; ++it, i++)
    {
        StringCchCopyA(strs[i].buffer, strs[i].len, it->first.c_str());
    }
    return i;
}
WsBool IsExtensionAvailable_Plugin(const char *name)
{
    Lock lock(&PluginManager::mutex);
    return (PluginManager::inst().GetExtensionFactories().find(name) != PluginManager::inst().GetExtensionFactories().end() ? WSTRUE : WSFALSE);
}

WsExtension GetExtension_Plugin(const struct _PluginInterface *pi, const char *name)
{
    if(name == 0 || pi == 0)
        return 0;

    Lock lock(&PluginManager::mutex);
    map<string, ExtensionFactory>::iterator extFactory = PluginManager::inst().GetExtensionFactories().find(name);

    if(extFactory != PluginManager::inst().GetExtensionFactories().end())
    {
        WsExtension ext = extFactory->second.Create();
        if(ext == 0)
            return 0;

        if(pi->data->extensionCache.find(ext) != pi->data->extensionCache.end())
        {
            extFactory->second.Destroy(ext);
            return 0;
        }
        pi->data->extensionCache[ext] = extFactory->second;
        return ext;
    }
    else
    {
        return 0;
    }
}

WsBool ReleaseExtension_Plugin(const struct _PluginInterface *pi, WsExtension ext)
{
    if(ext == 0 || pi == 0)
        return WSFALSE;

    std::map<WsExtension, ExtensionFactory>::iterator extFactory = pi->data->extensionCache.find(ext);

    if(extFactory != pi->data->extensionCache.end())
    {
        extFactory->second.Destroy(ext);
        pi->data->extensionCache.erase(extFactory);
        return WSTRUE;
    }
    else
    {
        return WSFALSE;
    }
}

WsHandle HookFunction_Plugin(const struct _PluginInterface *pi, void *oldAddress, void *newAddress)
{
    Lock lock(&PluginManager::mutex);
    try
    {
        std::shared_ptr<HookState> newhs(new HookState);

        *(newhs.get()) = Hookah::inst().HookFunction(oldAddress, newAddress);

        pi->data->hookStateCache.insert(newhs);

        return (WsHandle)newhs.get();
    }
    catch(const AlreadyHookedException&)
    {
        return 0;
    }
    
}
struct FindByPointer
{
    HookState *target;
    FindByPointer(HookState *target) : target(target)
    {

    }
    bool operator ()(const std::shared_ptr<HookState> &hs)
    {
        return (target == hs.get());
    }
};
WsBool UnhookFunction_Plugin(const struct _PluginInterface *pi, WsHandle handle)
{
    Lock lock(&PluginManager::mutex);
    WsBool unhooked = (Hookah::inst().UnhookFunction(*((HookState*)handle)) ? WSTRUE : WSFALSE);
    lock.Unlock();

    std::set<std::shared_ptr<HookState> >::iterator it = std::find_if(pi->data->hookStateCache.begin(), 
                                                                      pi->data->hookStateCache.end(), 
                                                                      FindByPointer((HookState*)handle));
    if(it != pi->data->hookStateCache.end())
    {
        pi->data->hookStateCache.erase(it);
    }

    return unhooked;
}
WsBool IsHooked_Plugin(void *oldAddress)
{
    return (Hookah::inst().IsHooked(oldAddress) ? WSTRUE : WSFALSE);
}
void *GetOriginalFunction_Plugin(WsHandle handle)
{
    return ((HookState*)handle)->GetOldAddress();
}

WsBool GetAllocationBase_Plugin(AllocationInfo *allocInfo, void *addr)
{
    MEMORY_BASIC_INFORMATION mem;

    if(addr==0 || allocInfo==0)
        return WSFALSE; // GetDllMemInfo failed!pAddr

    if(!VirtualQuery(addr, &mem, sizeof(mem)))
        return WSFALSE;

    allocInfo->base = mem.AllocationBase;

    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)mem.AllocationBase;
    UINT_PTR cdos = (UINT_PTR)dos;
    IMAGE_NT_HEADERS *pe = (IMAGE_NT_HEADERS*)(cdos + dos->e_lfanew);

    if(pe->Signature == IMAGE_NT_SIGNATURE) 
    {
        allocInfo->size = pe->OptionalHeader.SizeOfImage;
    }
    else
    {
        allocInfo->size = mem.RegionSize;
    }

    return WSTRUE;
}

ProtectionMode VirtualProtect_Plugin(void *address, size_t size, ProtectionMode newProtection)
{
    ProtectionMode oldProt;
    if(::VirtualProtect(address, size, (DWORD)newProtection, (DWORD*)&oldProt))
        return oldProt;
    else
        return PROTECTION_MODE_NOACCESS;
}

void * FindSignature_Plugin(const AllocationInfo *allocInfo, const char *sig)
{
    // eat up whitespaces
    std::string sigStr = clean(std::string(sig), std::string(" \r\n\t"));
    if((sigStr.length() % 2) == 1) 
        return 0; // the sig length is not an even number

    unsigned char *endPtr = ((unsigned char*)allocInfo->base)+allocInfo->size;
    for(unsigned char *curPtr = (unsigned char*)allocInfo->base; curPtr < endPtr; curPtr++) 
    {
        if(IsBadReadPtr(curPtr, 1))
            continue;

        unsigned char *innerPtr = curPtr;
        std::string::iterator it;
        for(it = sigStr.begin(); it != sigStr.end(); std::advance(it, 2),innerPtr++)
        {
            char b1 = *it;
            char b2 = *(it+1);

            if(b1 == '?' && b2 == '?')
            {
                continue;
            }
            else
            {
                int c1, c2;

                bool firstok =  false;
                bool secondok = false;

                if(b1 >= '0' && b1 <= '9')
                {
                    c1 = (b1 - '0'); firstok = true;
                }
                else if(b1 >= 'A' && b1 <= 'F')
                {
                    c1 = (b1 - 'A')+10; firstok = true;
                }
                else if(b1 >= 'a' && b1 <= 'f')
                {
                    c1 = (b1 - 'a')+10; firstok = true;
                }

                if(b2 >= '0' && b2 <= '9')
                {
                    c2 = (b2 - '0'); secondok = true;
                }
                else if(b2 >= 'A' && b2 <= 'F')
                {
                    c2 = (b2 - 'A')+10; secondok = true;
                }
                else if(b2 >= 'a' && b2 <= 'f')
                {
                    c2 = (b2 - 'a')+10; secondok = true;
                }

                if(firstok && secondok)
                {
                    int num = c1*16 + c2;
                    if(num != *innerPtr)
                        break;
                }
                else
                    return 0; //malformed sig
            }
        }
        if(std::distance(sigStr.begin(), it) == sigStr.length())
        {
            // sig found
            return curPtr;
        }
    }

    return 0;
}
}


PluginInterfaceData::~PluginInterfaceData()
{
    for (std::map<WsExtension, ExtensionFactory>::const_iterator it = extensionCache.begin(); it != extensionCache.end(); ++it)
    {
        it->second.Destroy(it->first);
    }

    // clean up events
    for(map<string, set<EventFunctionData*> >::const_iterator it = registerEvents.begin();
        it != registerEvents.end(); ++it)
    {
        set<EventFunctionData*> &globalEvents = PluginManager::inst().GetEvents()[it->first];

        for(set<EventFunctionData*>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
        {
            globalEvents.erase(*it2);
            delete *it2;
        }

    }
    for(set<std::shared_ptr<HookState> >::iterator it = hookStateCache.begin(); it != hookStateCache.end(); ++it)
    {
        Hookah::inst().UnhookFunction(*(it->get()));
    }
}

PluginInterfaceWrapper::PluginInterfaceWrapper()
{
    pluginInterface.data = new PluginInterfaceData;
    pluginInterface.log = new LoggingInterface;
    pluginInterface.manager = new PluginManagerInterface;
    pluginInterface.system = new SystemInterface;
    pluginInterface.event = new EventInterface;
    pluginInterface.extension = new ExtensionInterface;
    pluginInterface.hook = new HookInterface;
    pluginInterface.memory = new MemoryInterface;

    pluginInterface.data->moduleHandle = 0;

    pluginInterface.data->log.SetLogLevel(Log::INFO);
    LogFactory::inst().SetupConsoleLog(pluginInterface.data->log);

    pluginInterface.log->Debug = (WsHandle)Log::DEBUG;
    pluginInterface.log->Info = (WsHandle)Log::INFO;
    pluginInterface.log->Warn = (WsHandle)Log::WARN;
    pluginInterface.log->Error = (WsHandle)Log::ERROR;
    pluginInterface.log->Fatal = (WsHandle)Log::FATAL;
    pluginInterface.log->SetLogLevel = SetLogLevel_Plugin;
    pluginInterface.log->GetLogLevel = GetLogLevel_Plugin;
    pluginInterface.log->Log = Log_Plugin;
    pluginInterface.log->Log2 = Log2_Plugin;

    pluginInterface.manager->GetUnloadedPluginList = GetUnloadedPluginList_Plugin;
    pluginInterface.manager->GetLoadedPluginList = GetLoadedPluginList_Plugin;
    pluginInterface.manager->Load = LoadPlugin_Plugin;
    pluginInterface.manager->Unload = UnloadPlugin_Plugin;
    pluginInterface.manager->Reload = ReloadPlugin_Plugin;

    pluginInterface.system->GetModuleHandle = GetModuleHandle_Plugin;
    pluginInterface.system->GetProcessInformation = GetProcessInformation_Plugin;
    pluginInterface.system->GetModuleBase = GetModuleBase_Plugin;
    pluginInterface.system->GetProcAddress = GetProcAddress_Plugin;
    pluginInterface.system->RaiseException = RaiseException_Plugin;

    pluginInterface.event->SubscribeEvent = SubscribeEvent_Plugin;
    pluginInterface.event->GetEventList = GetEventList_Plugin;
    pluginInterface.event->IsEventAvailable = IsEventAvailable_Plugin;
    pluginInterface.event->UnsubscribeEvent = UnsubscribeEvent_Plugin;
    pluginInterface.event->IsEventSubscribed = IsEventSubscribed_Plugin;

    pluginInterface.hook->HookFunction = HookFunction_Plugin;
    pluginInterface.hook->UnhookFunction = UnhookFunction_Plugin;
    pluginInterface.hook->IsHooked = IsHooked_Plugin;
    pluginInterface.hook->GetOriginalFunction = GetOriginalFunction_Plugin;

    pluginInterface.memory->FindSignature = FindSignature_Plugin; 
    pluginInterface.memory->GetAllocationBase = GetAllocationBase_Plugin;
    pluginInterface.memory->VirtualProtect = VirtualProtect_Plugin;

    pluginInterface.extension->GetExtensionList = GetExtensionList_Plugin;
    pluginInterface.extension->IsExtensionAvailable = IsExtensionAvailable_Plugin;
    pluginInterface.extension->GetExtension = GetExtension_Plugin;
    pluginInterface.extension->ReleaseExtension = ReleaseExtension_Plugin;
}

PluginInterfaceWrapper::~PluginInterfaceWrapper()
{
    delete pluginInterface.data;
    delete pluginInterface.log;
    delete pluginInterface.manager;
    delete pluginInterface.system;
    delete pluginInterface.event;
    delete pluginInterface.extension;
    delete pluginInterface.hook;
    delete pluginInterface.memory;
}

Plugin::Plugin(const std::string &dir, const std::string &name)
: name(name), dir(dir)
{//Sets up function pointers for the plugins.

}

Plugin::~Plugin()
{

}

void Plugin::InternalLoad()
{
    // plugins are forward compatible, 
    // that means newer version plugins do not work with older versions of packet editor
    // while older version plugins work with newer version of packet editor
    if(Load() > PACKET_EDITOR_VERSION)
    {
        throw WSException("This plugin is compiled with a newer version of packet editor and "  
                           "is not supported with the current version of packet editor");
    }
}

void Plugin::InternalUnload()
{
    Unload();
}

const std::string &Plugin::GetFileDir() const
{
    return dir;
}
std::string Plugin::GetFilePath() const
{
    return dir + name;
}
const std::string &Plugin::GetFileName() const
{
    return name;
}
PluginInterface & Plugin::GetPluginInterface()
{
    return pluginInterface;
}



