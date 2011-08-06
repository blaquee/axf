#ifndef wsthread_h__
#define wsthread_h__

struct Lock
{
    bool locked;
    LPCRITICAL_SECTION mutex; 
    Lock(LPCRITICAL_SECTION mutex)
        : mutex(mutex),locked(true)
    {
        EnterCriticalSection(mutex);
    }
    ~Lock()
    {
        Unlock();
    }

    void Unlock()
    {
        if(locked)
        {
            locked = false;
            LeaveCriticalSection(mutex);
        }
    }
};

#endif // wsthread_h__