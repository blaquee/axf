#include <Python.h>
#include <CXX/Objects.hxx>
#include <CXX/Extensions.hxx>
#include <CXX/Exception.hxx>
#include <string>
#include "extensionapi.h"

AXF_EXTENSION_DESCRIPTION(AXF_MAKE_VERSION(1,0,0), OnInitExtension, "Python", "Hunter", "Provides python scripting support")

static PluginInterfaceEx pi;
static ExtenderInterfaceEx ei;
static std::wstring pypath;
static std::wstring pyhome;

// initialized in OnInitExtension
static Py::Module traceback(0);
static Py::Module sys(0);
static bool isLoading = false;

struct AutoRelease
{
    PluginBinary script;
    PluginData pluginData;
    AutoRelease(PluginData pluginData, PluginBinary script) : pluginData(pluginData), script(script) {}
    ~AutoRelease()
    {
        pluginData.ReleaseBinary(&script);
    }
};

struct ScriptContext
{
    Py::Dict globals;
    Py::Module module;
    std::string name;

    ScriptContext(const std::string &modName)
        : module(modName), name(modName)
    {
        globals = module.getDict();
        globals["__builtins__"] = Py::Module::Import("builtins");
    }
};

struct GILLock
{
    PyGILState_STATE gil;
    GILLock()
    {
        gil = PyGILState_Ensure();
    }
    ~GILLock()
    {
        PyGILState_Release(gil);
    }
};

template<typename T>
struct ResetVarOnExit
{
    T oldVal;
    T *var;
    ResetVarOnExit(T *var, T newVal)
    {
        this->var = var;
        oldVal = *var;
        *var = newVal;
    }
    ~ResetVarOnExit()
    {
        *this->var = oldVal;
    }
};
struct IsSymbol
{
    bool operator ()(char c)
    {
        return !isalnum(c);
    }
};

static std::wstring s2wspath(const std::string& s)
{
    const int slength = (int)s.length() + 1;
    const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), slength, 0, 0); 
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

static void OnUnloadPlugin(AxfHandle clientHandle)
{
    if(isLoading == false)
    {
        ResetVarOnExit<bool> resetter(&isLoading, true);
        GILLock gil;
        ScriptContext *ctx = (ScriptContext*)clientHandle;
        // also delete the module from python
        Py::Dict mods(sys.getAttr("modules"));
        mods.delItem(ctx->name);
        delete ctx;
    }
}

static void OnLoadPlugin(void *arg)
{
    PluginData *plugin = (PluginData*)arg;
    PluginBinary script;

    if(isLoading==false && std::strcmp(plugin->fileExtension, "py") == 0 && plugin->GetBinary(plugin, &script))
    {
        ResetVarOnExit<bool> resetter(&isLoading, true);
        GILLock gil;
        AutoRelease rls(*plugin, script);

        std::string nameStr = plugin->name;
        std::replace_if(nameStr.begin(), nameStr.end(), IsSymbol(), '_');
        ScriptContext *ctx = new ScriptContext(nameStr);

        try
        {
            Py::Object scriptRes(PyRun_String((char*)script.data, Py_file_input, *ctx->globals, *ctx->globals), true);
            if(scriptRes.isNull())
            {
                throw Py::Exception();
            }

            Py::Object mainFunc = ctx->module.getAttr("Main");
            if(mainFunc.isCallable())
            {
                Py::Callable mainCall = mainFunc;
                Py::Object res = mainCall.apply(Py::TupleN());
                if(!res.isNull())
                {
                    if(res.isType(Py::Long().type()))
                    {
                        plugin->initSuccess = AXFTRUE;
                        plugin->clientVersion = Py::Long(res); // required for version compatibility
                        plugin->clientHandle = (AxfHandle)ctx; 
                        plugin->OnPluginUnload = OnUnloadPlugin; // make the custom plugin unloadable
                        return;
                    }
                    else
                    {
                        throw Py::Exception("Main function must return the plugin version (an int)");
                    }
                }
                else
                {
                    throw Py::Exception();
                }
            }
            else 
            {
                throw Py::Exception("Can't find the Main function");
            }
        } 
        catch(const Py::Exception &)
        {
            PyObject *ptype, *pvalue, *ptrace;
            PyErr_Fetch( &ptype, &pvalue, &ptrace );
            PyErr_NormalizeException(&ptype, &pvalue, &ptrace);
            Py::Object msgList = ptrace==0 ? traceback.callMemberFunction("format_exception_only", Py::TupleN(Py::asObject(ptype), Py::asObject(pvalue))) : 
                                             traceback.callMemberFunction("format_exception", Py::TupleN(Py::asObject(ptype), Py::asObject(pvalue), Py::asObject(ptrace)));
            Py::String msg = Py::String().callMemberFunction("join", Py::TupleN(msgList));
            pi.Log(pi.Error(), msg);
            delete ctx;
        }
    }
    plugin->initSuccess = AXFFALSE;
    plugin->clientHandle = 0;
    plugin->OnPluginUnload = 0;
}

static bool SetupScriptEngine()
{
    traceback = Py::Module::Import("traceback");
    sys = Py::Module::Import("sys");

    Py::Object path = sys.getAttr("path");

    Py::List pathSeq = path;
    for(Py::Sequence::iterator it = pathSeq.begin(); it != pathSeq.end(); ++it)
    {
        pi.Log(pi.Info(), it->str());
    }

    // a hack to make ctypes work
    sys.setAttr("dllhandle", Py::Long(ULONG_PTR(pi.GetModuleHandle())));

    pi.SubscribeEvent(ON_LOAD_PLUGIN, &OnLoadPlugin);
    return true;
}

static AxfBool OnInitExtension(const PluginInterface *p, const ExtenderInterface *e)
{
    pi = p;
    ei = e;

    pypath = s2wspath(pi.GetBaseDirectory() + "\\python\\python.zip") + std::wstring(L";") +
             s2wspath(pi.GetBaseDirectory() + "\\python\\lib") + std::wstring(L";") +
             s2wspath(pi.GetPluginDirectory());
    pyhome = s2wspath(pi.GetBaseDirectory() + "\\python");

    Py_SetProgramName(L"AXF");
    Py_SetPythonHome((wchar_t*)pyhome.c_str());
    Py_SetPath((wchar_t*)pypath.c_str());
    Py_InitializeEx(0);
    PyEval_InitThreads();
    PyThreadState *pts = PyGILState_GetThisThreadState();

    if(SetupScriptEngine())
    {
        std::ostringstream ss;
        ss << "Loaded Python version: " << Py_GetVersion();
        pi.Log(pi.Info(), ss.str());

        PyEval_ReleaseThread(pts);
        return AXFTRUE;
    }
    else
    {
        PyEval_ReleaseThread(pts);
        return AXFFALSE;
    }
}
