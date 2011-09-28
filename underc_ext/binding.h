#ifndef binding_h__
#define binding_h__

#define NAMESPACE "pi_"

#ifdef WIN32
#define _STDCALL_ __stdcall
#else
#define _STDCALL_
#endif

// globals
PluginInterfaceEx pi=0;

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

 void* _STDCALL_ GetModuleHandle() 
{
    return pi.GetPluginInterface().system->GetModuleHandle(pi.GetPluginInterfacePtr());
}
 ProcessInfo _STDCALL_ GetProcessInformation() 
{
    ProcessInfo info;
    pi.GetPluginInterface().system->GetProcessInformation(&info);

    return info;
}
 void * _STDCALL_ GetModuleBase(const std::string &name) 
{
    return pi.GetPluginInterface().system->GetModuleBase(name.c_str());
}
 void * _STDCALL_ GetProcAddress(void *base, const std::string &name) 
{
    return pi.GetPluginInterface().system->GetProcAddress(base, name.c_str());
}

/* Throws an exception to the plugin manager, dataUnused must be set to NULL for now  */
 void _STDCALL_ RaiseException(const std::string &exceptionMsg="Unknown Exception Raised!", void *dataUnused=0) 
{
    pi.GetPluginInterface().system->RaiseException(exceptionMsg.c_str(), dataUnused);
}

 std::string _STDCALL_ GetAboutMessage() 
{
    return pi.GetPluginInterface().system->GetAboutMessage();
}
 unsigned int _STDCALL_ GetVersion() 
{
    return pi.GetPluginInterface().system->GetVersion();
}


 std::string  _STDCALL_ GetBaseDirectory() 
{
    return GetDirectory(pi.GetPluginInterface().system->GetBaseDirectory);
        
}
 std::string _STDCALL_ GetPluginDirectory() 
{
    return GetDirectory(pi.GetPluginInterface().system->GetPluginDirectory);
}
 std::string _STDCALL_ GetExtensionDirectory() 
{
    return GetDirectory(pi.GetPluginInterface().system->GetExtensionDirectory);
}



 LogLevel _STDCALL_ Quiet() 
{
    return pi.GetPluginInterface().log->Quiet;
}
 LogLevel _STDCALL_ Debug() 
{
    return pi.GetPluginInterface().log->Debug;
}
 LogLevel _STDCALL_ Info() 
{
    return pi.GetPluginInterface().log->Info;
}
 LogLevel _STDCALL_ Warn() 
{
    return pi.GetPluginInterface().log->Warn;
}
 LogLevel _STDCALL_ Error() 
{
    return pi.GetPluginInterface().log->Error;
}

 void _STDCALL_ SetLogLevel(const LogLevel type) 
{
    pi.GetPluginInterface().log->SetLogLevel(pi.GetPluginInterfacePtr(), type);
}
 LogLevel _STDCALL_ GetLogLevel() 
{
    return pi.GetPluginInterface().log->GetLogLevel(pi.GetPluginInterfacePtr());
}
 void _STDCALL_ Log(const std::string &s) 
{
    pi.GetPluginInterface().log->Log(pi.GetPluginInterfacePtr(), s.c_str());
}
 void _STDCALL_ Log2(const LogLevel type, const std::string &s) 
{
    pi.GetPluginInterface().log->Log2(pi.GetPluginInterfacePtr(), type, s.c_str());
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



 std::vector<std::string> _STDCALL_ GetUnloadedPluginList() 
{
    return GetPluginList(pi.GetPluginInterface().manager->GetUnloadedPluginList);
}
 std::vector<std::string>  _STDCALL_ GetLoadedPluginList() 
{
    return GetPluginList(pi.GetPluginInterface().manager->GetLoadedPluginList);
}

 WsBool _STDCALL_ LoadPlugin(const std::string &fileName) 
{
    return pi.GetPluginInterface().manager->Load(fileName.c_str());
}
 WsBool _STDCALL_ UnloadPlugin(const std::string &fileName) 
{
    return pi.GetPluginInterface().manager->Unload(fileName.c_str());
}
 WsBool _STDCALL_ ReloadPlugin(const std::string &fileName) 
{
    return pi.GetPluginInterface().manager->Reload(fileName.c_str());
}




 std::vector<std::string> _STDCALL_ GetEventList() 
{
    size_t sizeOfStrs = pi.GetPluginInterface().event->GetEventList(0, 0);
    String *strs = AllocString(2048, sizeOfStrs);
    pi.GetPluginInterface().event->GetEventList(strs, sizeOfStrs);

    std::vector<std::string> results;
    results.resize(sizeOfStrs);
    std::transform(strs, strs+sizeOfStrs, results.begin(), &StringToStdString);

    FreeString(strs, sizeOfStrs);

    return results;
}
 WsBool _STDCALL_ IsEventAvailable(const std::string &eventName) 
{
    return pi.GetPluginInterface().event->IsEventAvailable(eventName.c_str());
}

/* all event functions are __cdecl call convention */
/* returns NULL handle on failure */
 WsHandle _STDCALL_ SubscribeEvent(const std::string &eventName, EventFunction eventFunc) 
{
    return pi.GetPluginInterface().event->SubscribeEvent(pi.GetPluginInterfacePtr(), eventName.c_str(), eventFunc);
}

/* You do not have to manually remove the event in OnUnload(), 
    the plugin manager will take care of cleaning up events */
 void _STDCALL_ UnsubscribeEvent(WsHandle handle) 
{
    pi.GetPluginInterface().event->UnsubscribeEvent(pi.GetPluginInterfacePtr(), handle);
}

 WsBool _STDCALL_ IsEventSubscribed(WsHandle handle) 
{
    return pi.GetPluginInterface().event->IsEventSubscribed(pi.GetPluginInterfacePtr(), handle);
}

 WsHandle _STDCALL_ HookFunction(void *oldAddress, void *newAddress) 
{
    return pi.GetPluginInterface().hook->HookFunction(pi.GetPluginInterfacePtr(), oldAddress, newAddress);
}
 WsBool _STDCALL_ UnhookFunction(WsHandle handle) 
{
    return pi.GetPluginInterface().hook->UnhookFunction(pi.GetPluginInterfacePtr(), handle);
}
 WsBool _STDCALL_ IsHooked(void *oldAddress) 
{
    return pi.GetPluginInterface().hook->IsHooked(oldAddress);
}
 void * _STDCALL_ GetOriginalFunction(WsHandle handle) 
{
    return pi.GetPluginInterface().hook->GetOriginalFunction(handle);
}


    /*std::unique_ptr<AllocationInfo> GetAllocationBase(void *addr) const
    {
        std::unique_ptr<AllocationInfo> allocInfo(new AllocationInfo);
        WsBool isgood = pi.GetPluginInterface().memory->GetAllocationBase(allocInfo.get(), addr);
        if(isgood)
        {
            return allocInfo;
        }
        else
        {
            allocInfo.reset(0);
            return allocInfo;
        }
    }*/

 ProtectionMode _STDCALL_ VirtualProtect(void *address, size_t size, ProtectionMode newProtection) 
{
    return pi.GetPluginInterface().memory->VirtualProtect(address, size, newProtection);
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
 void * _STDCALL_ FindSignature(const AllocationInfo *allocInfo, const char *sig) 
{
    return pi.GetPluginInterface().memory->FindSignature(allocInfo, sig);
}



 std::vector<std::string> _STDCALL_ GetExtensionList() 
{
    size_t sizeOfStrs = pi.GetPluginInterface().extension->GetExtensionList(0, 0);
    String *strs = AllocString(2048, sizeOfStrs);
    pi.GetPluginInterface().extension->GetExtensionList(strs, sizeOfStrs);

    std::vector<std::string> results;
    results.resize(sizeOfStrs);
    std::transform(strs, strs+sizeOfStrs, results.begin(), &StringToStdString);

    FreeString(strs, sizeOfStrs);

    return results;
}

 WsBool  _STDCALL_ IsExtensionAvailable(const std::string &extName) 
{
    return pi.GetPluginInterface().extension->IsExtensionAvailable(extName.c_str());
}


WsHandle GetExtension(const std::string &extName) 
{
    return pi.GetPluginInterface().extension->GetExtension(pi.GetPluginInterfacePtr(), extName.c_str());
}

 WsBool _STDCALL_ ReleaseExtension(WsExtension ext) 
{
    return pi.GetPluginInterface().extension->ReleaseExtension(pi.GetPluginInterfacePtr(), ext);
}

} //namespace axf

#undef _STDCALL_

#endif // binding_h__

