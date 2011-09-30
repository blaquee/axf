#ifndef binding_h__
#define binding_h__

#define NAMESPACE "pi_"

#ifdef WIN32
#define _STDCALL_ __stdcall
#else
#define _STDCALL_
#endif

// globals

namespace axf
{


static std::string GetDirectory(size_t(*GetDirFunc)(String*)) 
{
    size_t len = GetDirFunc(0);
    String *s = AllocString(len, 1);
    GetDirFunc(s);
    std::string res = s->buffer;
    FreeString1(s);

    return res;
}

 void* _STDCALL_ GetModuleHandle(PluginInterface *pi) 
{
    return pi->system->GetModuleHandle(pi);
}
 void _STDCALL_ GetProcessInformation(PluginInterface *pi, ProcessInfo *pInfo) 
{
    pi->system->GetProcessInformation(pInfo);
}
 void * _STDCALL_ GetModuleBase(PluginInterface *pi, const char *name) 
{
    return pi->system->GetModuleBase(name);
}
 void * _STDCALL_ GetProcAddress(PluginInterface *pi, void *base, const char *name) 
{
    return pi->system->GetProcAddress(base, name);
}

/* Throws an exception to the plugin manager, dataUnused must be set to NULL for now  */
 void _STDCALL_ RaiseException(PluginInterface *pi, const char *exceptionMsg, void *dataUnused) 
{
    pi->system->RaiseException(exceptionMsg, dataUnused);
}

const char* _STDCALL_ GetAboutMessage(PluginInterface *pi) 
{
    return pi->system->GetAboutMessage();
}
 unsigned int _STDCALL_ GetVersion(PluginInterface *pi) 
{
    return pi->system->GetVersion();
}


size_t  _STDCALL_ GetBaseDirectory(PluginInterface *pi, String *s) 
{
    return pi->system->GetBaseDirectory(s);
        
}
size_t _STDCALL_ GetPluginDirectory(PluginInterface *pi, String *s) 
{
    return pi->system->GetPluginDirectory(s);
}
size_t _STDCALL_ GetExtensionDirectory(PluginInterface *pi, String *s) 
{
    return pi->system->GetExtensionDirectory(s);
}



 LogLevel _STDCALL_ Quiet(PluginInterface *pi) 
{
    return pi->log->Quiet;
}
 LogLevel _STDCALL_ Debug(PluginInterface *pi) 
{
    return pi->log->Debug;
}
 LogLevel _STDCALL_ Info(PluginInterface *pi) 
{
    return pi->log->Info;
}
 LogLevel _STDCALL_ Warn(PluginInterface *pi) 
{
    return pi->log->Warn;
}
 LogLevel _STDCALL_ Error(PluginInterface *pi) 
{
    return pi->log->Error;
}

 void _STDCALL_ SetLogLevel(PluginInterface *pi, const LogLevel type) 
{
    pi->log->SetLogLevel(pi, type);
}
 LogLevel _STDCALL_ GetLogLevel(PluginInterface *pi) 
{
    return pi->log->GetLogLevel(pi);
}
 void _STDCALL_ Log(PluginInterface *pi, const char *s) 
{
    pi->log->Log(pi, s);
}
 void _STDCALL_ Log2(PluginInterface *pi, const LogLevel type, const char *s) 
{
    pi->log->Log2(pi, type, s);
}



static std::vector<std::string> GetPluginList(size_t (*GetPList)(String *strs, size_t sizeofStrs)) 
{
    size_t sizeOfStrs = GetPList(0, 0);
    String *strs = AllocString(2048, sizeOfStrs);
    GetPList(strs, sizeOfStrs);

    std::vector<std::string> results;
    results.resize(sizeOfStrs);
    std::transform(strs, strs+sizeOfStrs, results.begin(), &StringToStdString);

    FreeString(strs, sizeOfStrs);

    return results;
}



 size_t _STDCALL_ GetUnloadedPluginList(PluginInterface *pi, String *strs, size_t numOfStrs) 
{
    return pi->manager->GetUnloadedPluginList(strs, numOfStrs);
}
size_t  _STDCALL_ GetLoadedPluginList(PluginInterface *pi, String *strs, size_t numOfStrs) 
{
    return pi->manager->GetLoadedPluginList(strs, numOfStrs);
}

 WsBool _STDCALL_ LoadPlugin(PluginInterface *pi, const char *fileName) 
{
    return pi->manager->Load(fileName);
}
 WsBool _STDCALL_ UnloadPlugin(PluginInterface *pi, const char *fileName) 
{
    return pi->manager->Unload(fileName);
}
 WsBool _STDCALL_ ReloadPlugin(PluginInterface *pi, const char *fileName) 
{
    return pi->manager->Reload(fileName);
}




size_t _STDCALL_ GetEventList(PluginInterface *pi, String *strs, size_t numOfStrs) 
{
    return pi->event->GetEventList(strs, numOfStrs);
}
 WsBool _STDCALL_ IsEventAvailable(PluginInterface *pi, const char *eventName) 
{
    return pi->event->IsEventAvailable(eventName);
}

/* all event functions are __cdecl call convention */
/* returns NULL handle on failure */
 WsHandle _STDCALL_ SubscribeEvent(PluginInterface *pi, const char *eventName, EventFunction eventFunc) 
{
    return pi->event->SubscribeEvent(pi, eventName, eventFunc);
}

/* You do not have to manually remove the event in OnUnload(), 
    the plugin manager will take care of cleaning up events */
 void _STDCALL_ UnsubscribeEvent(PluginInterface *pi, WsHandle handle) 
{
    pi->event->UnsubscribeEvent(pi, handle);
}

 WsBool _STDCALL_ IsEventSubscribed(PluginInterface *pi, WsHandle handle) 
{
    return pi->event->IsEventSubscribed(pi, handle);
}

 WsHandle _STDCALL_ HookFunction(PluginInterface *pi, void *oldAddress, void *newAddress) 
{
    return pi->hook->HookFunction(pi, oldAddress, newAddress);
}
 WsBool _STDCALL_ UnhookFunction(PluginInterface *pi, WsHandle handle) 
{
    return pi->hook->UnhookFunction(pi, handle);
}
 WsBool _STDCALL_ IsHooked(PluginInterface *pi, void *oldAddress) 
{
    return pi->hook->IsHooked(oldAddress);
}
 void * _STDCALL_ GetOriginalFunction(PluginInterface *pi, WsHandle handle) 
{
    return pi->hook->GetOriginalFunction(handle);
}


WsBool _STDCALL_ GetAllocationBase(PluginInterface *pi, AllocationInfo *allocInfo, void *addr)
{
    return pi->memory->GetAllocationBase(allocInfo, addr);
}

ProtectionMode _STDCALL_ VirtualProtect(PluginInterface *pi, void *address, size_t size, ProtectionMode newProtection) 
{
    return pi->memory->VirtualProtect(address, size, newProtection);
}

/*
    The signature is formatted as following:
    1. Groups of 2 characters long hexcode separated by arbitrary amount whitespaces (tab, spacebar, newline)
    2. a 2 characters question mark (??) represents a wildcard, it will be ignored during the scanning

    Examples:
    char *sig = "1F 01 00 B9 FF ?? AB ?? ?? ?? ?? EF";

    // you can also use newlines
    char *sig = "1F 01 00 B9 FA ?? AB ?? ?? ?? ?? EF"
                "2F 02 00 B9 FB ?? AB ?? ?? ?? ?? EF"
                "3F 03 00 B9 FC ?? AB ?? ?? ?? ?? EF";

    // you can also use irregular spacing, mixed with tabs
    char *sig = "1F      01 00 B9 FA ??     AB ?? ?? ?? ?? EF" 
                "2F 02 00 B9 FB ?? AB ?? ??     ?? ?? EF"
                "3F 03 00 B9 FC ?? AB               ?? ?? ?? ?? EF";

    Returns null if the function fails to find the signature
*/
 void * _STDCALL_ FindSignature(PluginInterface *pi, const AllocationInfo *allocInfo, const char *sig) 
{
    return pi->memory->FindSignature(allocInfo, sig);
}



size_t _STDCALL_ GetExtensionList(PluginInterface *pi, String *strs, size_t numOfStrs) 
{
    return pi->extension->GetExtensionList(strs, numOfStrs);
}

 WsBool  _STDCALL_ IsExtensionAvailable(PluginInterface *pi, const char *extName) 
{
    return pi->extension->IsExtensionAvailable(extName);
}


WsHandle GetExtension(PluginInterface *pi, const char *extName) 
{
    return pi->extension->GetExtension(pi, extName);
}

 WsBool _STDCALL_ ReleaseExtension(PluginInterface *pi, WsExtension ext) 
{
    return pi->extension->ReleaseExtension(pi, ext);
}

} //namespace axf

#undef _STDCALL_

#endif // binding_h__

