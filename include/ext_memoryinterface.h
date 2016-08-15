#ifndef ext_memory_h__
#define ext_memory_h__

#define MEMORY_INTERFACE_1 "MemoryInterface_1"

typedef struct _MemoryInterface1
{
    /* 
     param 1 = target address
     param 2 = source buffer
     param 3 = size
     
     returns the amount of data written, or -1 if it failed
    */
    int (*Write)(void*, const void*, unsigned int);

    /*
     param 1 = target address
     param 2 = source buffer
     param 3 = size
     
     returns the amount of data read, or -1 if it failed
    */
    int (*Read)(const void*, void*, unsigned int);
} MemoryInterface1;

#ifdef __cplusplus
class MemoryInterfaceEx1
{

    MemoryInterface1 *ext;

public:
    MemoryInterfaceEx1(MemoryInterface1 *ext) : ext(ext)
    {
    }
    operator AxfExtension()
    {
        return (AxfExtension)ext;
    }
    int Write(void *target, const void *src, unsigned int size) const
    {
        return ext->Write(target, src, size);
    }
    int Read(const void *target, void *src, unsigned int size) const
    {
        return ext->Read(target, src, size);
    }

    // templated version
    template<typename T>
    int Write(void *target, const T *src) const
    {
        return ext->Write(target, src, sizeof(T));
    }

    template<typename T>
    int Read(const void *target, T *src) const
    {
        return ext->Read(target, src, sizeof(T));
    }
};
#endif

#endif // ext_memory_h__

