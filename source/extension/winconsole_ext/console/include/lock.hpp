#pragma once

namespace db
{
    class lock {
    public:
        lock() { InitializeCriticalSection(&_lock); }
        ~lock() { DeleteCriticalSection(&_lock); }
        void acquire() { EnterCriticalSection(&_lock); }
        void release() { LeaveCriticalSection(&_lock); }

    private:
        CRITICAL_SECTION _lock;
    };
}
