#include <Windows.h>
#include "extensionapi.h"
#include "ext_memoryinterface.h"

AXF_EXTENSION_DESCRIPTION(AXF_MAKE_VERSION(1,0,0), OnInitExtension, "Memory Modder", "Hunter", "Modify the memory of your process with this extension")


static PluginInterfaceEx pi;
static ExtenderInterfaceEx ei;

inline int MemoryCopy(void* target, void* src, unsigned int size)
{
    if(size <= 0)
        return 0;
    if(IsBadWritePtr(target, size) || IsBadReadPtr(src, size))
        return -1;

    memcpy(target, src, size);

    return size;
}

static int WriteMemory(void* target, const void* src, unsigned int size)
{
    return MemoryCopy(target, (void*)src, size);
}

static int ReadMemory(const void* target, void* src, unsigned int size)
{
    return MemoryCopy(src, (void*)target, size);
}

namespace MemoryInterfaceFactory1
{
    static WsExtension Create()
    {
        MemoryInterface1 *ext = new MemoryInterface1;
        ext->Write = &WriteMemory;
        ext->Read = &ReadMemory;

        return (WsExtension)ext;
    }
    static void Destroy(WsExtension ext)
    {
        MemoryInterface1 *deleteme = (MemoryInterface1*)ext;
        delete deleteme;
    }
}


static WsBool OnInitExtension(const struct _PluginInterface *p, const struct _ExtenderInterface *e)
{
    pi = p;
    ei = e;

    ExtensionFactory memExt = { &MemoryInterfaceFactory1::Create, &MemoryInterfaceFactory1::Destroy };
    ei.AddExtension(MEMORY_INTERFACE_1, &memExt);

    return WSTRUE;;
}

