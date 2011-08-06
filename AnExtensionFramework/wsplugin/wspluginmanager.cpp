#include "../pch.h"
#include "../hookah.h"
#include "../extensionman/extension.h"
#include "../extensionman/extensionplugin_c.h"
#include "wspluginmanager.h"
#include "wsplugin.h"
#include "wsexception.h"
#include "wsthread.h"
#include "regkey.h"


//#include "easyhook/easyhook.h"

// this list defines the names that the plugin manager should not show
// forbidden names must be in lowercase or else it won't work!
#define FORBIDDEN_PLUGINS { L"__init__.py", L"pluginapi.py" }

using namespace std;


CRITICAL_SECTION PluginManager::mutex;

static char pluginAppDir[2000];
static void InitPluginManager(PluginManager &pm)
{
    RegKey key(HKEY_CURRENT_USER);
    key.Open(REGISTRY_KEY);

    //PyEval_InitThreads();

    RegValue regDir = key[L"path"];

    WideCharToMultiByte(CP_UTF8, 0, regDir, wcslen(regDir), pluginAppDir, sizeof(pluginAppDir)-2, 0, 0);

    // this is located at app\plugin
    std::string appDir = pluginAppDir;
    std::string pluginDir = appDir + "\\plugin\\";
    std::string extDir = appDir + "\\extension\\";
    pm.SetExtensionDirectory(extDir);
    pm.SetPluginDirectory(pluginDir);
    pm.SetBaseDirectory(appDir);

    std::cout << "BaseDir: "<< appDir << std::endl;
    std::cout << "PluginDir: "<< pluginDir << std::endl;
}

PluginManager::PluginManager()
{
    srand((unsigned int) time(0));
    InitializeCriticalSection(&mutex);

    InitPluginManager(*this);

    events[ON_UNLOAD_EVENT];
}

PluginManager::~PluginManager()
{
    struct DeleteFunc
    {
        void operator()(Plugin *p) { delete p; }
    };
    
    for_each(loadedPlugins.begin(), loadedPlugins.end(), DeleteFunc());

    loadedPlugins.clear();
}

//////////////////////////////////////////////////////////////////////////

// comparison operator for std::find, it has to be global because the left hand operand is declared as a pointer
inline bool operator==(Plugin *lhs, const std::string &rhs )
{
    return (lhs->GetFilePath() == rhs);
}

std::string PluginManager::GetBaseDirectory() const
{
    return baseDir;
}

void PluginManager::SetBaseDirectory( const std::string &dir )
{
    baseDir = dir;
}

std::string PluginManager::GetExtensionDirectory() const
{
    return extDir;
}

void PluginManager::SetExtensionDirectory( const std::string &dir )
{
    extDir = dir;
}


void PluginManager::SetPluginDirectory( const std::string &dir )
{
    pluginDir = dir;
}

std::string PluginManager::GetPluginDirectory() const
{
    return pluginDir;
}

//////////////////////////////////////////////////////////////////////////
// Event Handling
map<string, set<EventFunctionData*> > & PluginManager::GetEvents()
{
    return events;
}

// Extensions
std::map<std::string, ExtensionFactory> & PluginManager::GetExtensionFactories()
{
    return extensionFactories;
}

namespace
{
    struct FindFileCloser
    {
        HANDLE h;
        FindFileCloser(HANDLE h): h(h){}
        ~FindFileCloser()
        {
            FindClose(h);
        }
    };
    template<typename T>
    struct ReleasableArray
    {
        T *p;
        ReleasableArray(T *p): p(p){}
        ~ReleasableArray(){ delete[] p; }
        inline operator T*() const
        {
            return p;
        }
        inline T *get() const
        {
            return p;
        }
    };

    inline std::string UTF8(const wchar_t *s)
    {
        int required = WideCharToMultiByte(CP_UTF8, 0, s, -1, 0, 0, 0,0);
        std::string newStr(required-1,0);
        WideCharToMultiByte(CP_UTF8, 0, s, -1, (char*)newStr.c_str(), newStr.size(), 0, 0);
        return newStr;
    }

    inline bool IsForbiddenName(const wchar_t *name,const  wchar_t **forbiddenNames, size_t numberOfForbiddenNames)
    {
        size_t len = wcslen(name);
        wchar_t *buffer = new wchar_t[len+1];
        StringCchCopyW(buffer,len+1, name);
        for(size_t i = 0;i < len;i++)
        {
            buffer[i] = ::towlower(buffer[i]);
        }
        for(size_t i = 0;i < numberOfForbiddenNames;i++)
        {
            if (wcsstr(buffer,forbiddenNames[i]) != 0)
            {
                delete[] buffer;
                return true;
            }
        }
        delete[] buffer;
        return false;
    }


    template<class StringType>
    void GetPluginList(std::vector<StringType> &pluginNames,const string &baseDir, const string &currentDir, const wchar_t **forbiddenNames, size_t sizeOfForbiddenNames)
    {


        WIN32_FIND_DATA wfd;
        string patternUTF8 = (currentDir == "") ? (baseDir + "*") : (baseDir + currentDir + "\\*");
        int required = MultiByteToWideChar(CP_UTF8, 0, patternUTF8.c_str(), patternUTF8.size(), 0, 0);
        ReleasableArray<wchar_t> pattern( new wchar_t[required+1] );
        memset(pattern, 0, (required+1)*sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8, 0, patternUTF8.c_str(), patternUTF8.size(), pattern, required);
        HANDLE h = FindFirstFile(pattern, &wfd);
        
        if(h == INVALID_HANDLE_VALUE)
        {
            wprintf(L"cannot open dir: %s len:%d\n", pattern.get(), required);
            return;
        }

        FindFileCloser fcloser(h);

        BOOL hasNext;
        do 
        {
            string utf8FileName = UTF8(wfd.cFileName);
            string currentFile = (currentDir == "") ? (utf8FileName) : (currentDir + "\\" + utf8FileName);

            if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                && utf8FileName[0] != '.' &&  utf8FileName[1] != '\0'
                && utf8FileName[0] != '.' && utf8FileName[1] != '.' && utf8FileName[2] != '\0')
            {
                GetPluginList(pluginNames, baseDir, currentFile, forbiddenNames, sizeOfForbiddenNames);
            }
            else
            {
                if(IsForbiddenName(wfd.cFileName, forbiddenNames, sizeOfForbiddenNames) == false)
                {
                    string::size_type pos = currentFile.find_last_of(".");
                    if(pos != string::npos)
                    {
                        string ext = currentFile.substr(pos+1);
                        if(ext == "py")
                        {
                            pluginNames.push_back(currentFile.c_str());
                        }
                        else if(ext == "dll")
                        {
                            pluginNames.push_back(currentFile.c_str());
                        }
                    }

                }
                    
            }

            hasNext = FindNextFile(h, &wfd);

        } while (hasNext);

    }
}


vector<string> PluginManager::GetPluginList() const
{
    vector<string> pluginNames;

    ::GetPluginList<string>(pluginNames, GetPluginDirectory(), "", 0, 0);

    return pluginNames;

}

std::vector<std::string> PluginManager::GetLoadedPluginList() const
{
    std::vector<std::string> list;
    for (std::vector<Plugin*>::const_iterator it = loadedPlugins.begin(); it != loadedPlugins.end(); ++it)
    {
        std::string pluginName = (*it)->GetFileName();

        list.push_back(pluginName);
    }
    return list;
}

std::vector<std::string> PluginManager::GetUnloadedPluginList() const
{
    std::vector<std::string> plugins = GetPluginList();
    std::set<std::string> pluginSet(plugins.begin(), plugins.end());

    for (std::vector<Plugin*>::const_iterator it = loadedPlugins.begin(); it != loadedPlugins.end(); ++it)
    {
        std::string pluginName = (*it)->GetFileName();

        if(pluginSet.find(pluginName) != pluginSet.end())
        {
            pluginSet.erase(pluginName);
        }
    }

    std::vector<std::string> unloadedPlugins(pluginSet.begin(), pluginSet.end());

    return unloadedPlugins;
}

void PluginManager::LoadExtension( const std::string &fileName )
{
    if(find(loadedPlugins.begin(), loadedPlugins.end(),GetExtensionDirectory() + fileName) != loadedPlugins.end())
    {
        throw WSException("Plugin is already loaded");
    }

    string::size_type pos = fileName.find_last_of(".");
    if(pos != string::npos)
    {
        string ext = fileName.substr(pos+1);
        if(ext == "dll")
        {
            ExtensionDllPlugin *dllPlugin = new ExtensionDllPlugin(GetExtensionDirectory(), fileName);
            try
            {
                dllPlugin->InternalLoad();
            }
            catch(...)
            {
                delete dllPlugin;
                throw;
            }

            loadedPlugins.push_back(dllPlugin);
        }
        else
        {
            throw WSException("not a plugin!");
        }
    }
    else
        throw WSException("not a plugin! no extension found");
}

void PluginManager::LoadPlugin( const std::string &fileName )
{

    Lock lock(&mutex);

    if(find(loadedPlugins.begin(), loadedPlugins.end(),GetPluginDirectory() + fileName) != loadedPlugins.end())
    {
        throw WSException("Plugin is already loaded");
    }

    string::size_type pos = fileName.find_last_of(".");
    if(pos != string::npos)
    {
        string ext = fileName.substr(pos+1);
        if(ext == "dll")
        {
            DllPlugin *dllPlugin = new DllPlugin(GetPluginDirectory(), fileName);
            try
            {
                dllPlugin->InternalLoad();
            }
            catch(...)
            {
                delete dllPlugin;
                throw;
            }

            loadedPlugins.push_back(dllPlugin);
        }
        else
        {
            throw WSException("not a plugin!");
        }
    }
    else
        throw WSException("not a plugin! no extension found");

}

void PluginManager::UnloadPlugin( const std::string &fileName )
{
    Lock lock(&mutex);
    vector<Plugin*>::iterator pluginIt = find(loadedPlugins.begin(), loadedPlugins.end(),  GetPluginDirectory() + fileName);

    if(pluginIt == loadedPlugins.end())
        throw WSException("Plugin is not loaded!\nfile:" + fileName);

    (*pluginIt)->InternalUnload();
    delete *pluginIt;
    loadedPlugins.erase(pluginIt);
}

void PluginManager::ReloadPlugin( const std::string &fileName )
{
    UnloadPlugin(fileName);
    LoadPlugin(fileName);
}

void PluginManager::SoftReloadPlugin( const std::string &fileName )
{
    Lock lock(&mutex);
    vector<Plugin*>::iterator pluginIt = find(loadedPlugins.begin(), loadedPlugins.end(), GetPluginDirectory() + fileName);

    if(pluginIt == loadedPlugins.end())
        throw WSException("Plugin is not loaded!\nfile:" + fileName);

    (*pluginIt)->SoftReload();
}
