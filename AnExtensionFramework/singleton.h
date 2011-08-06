#ifndef singleton_h__
#define singleton_h__

#include <stack>


class SingletonReleaser
{
    friend class SingletonStore;
    virtual void release() =0;
    virtual const char* getType() const =0;
};

class SingletonStore
{

    std::stack<SingletonReleaser*> singletons;

    FILE *logfile;
    int lognum;

public:
    SingletonStore() : logfile(0), lognum(1)
    {
#ifdef _DEBUG
        logfile = fopen("singleton_released.txt", "a");
        if(logfile)
        {
            fprintf(logfile,  "=========== Singleton log started ===========\n");
        }
#endif
    }
    ~SingletonStore()
    {  
        deleteAll();
#ifdef _DEBUG
        if(logfile)
        {
            fprintf(logfile,  "=========== Singleton log ended ===========\n");
            fclose(logfile);
        }
#endif
    }
    inline void addSingleton(SingletonReleaser *singleton)
    {
        singletons.push(singleton);
    }

    inline void deleteAll()
    {

        while(singletons.empty() == false)
        {

#ifdef _DEBUG
            if(logfile)
            {
                fprintf(logfile,  "[%d]Released: %s\n", lognum, singletons.top()->getType());
                lognum++;
            }
#endif
            singletons.top()->release();
            singletons.pop();

        }

    }
};

extern SingletonStore singletonStore;



template<class T>
class Singleton : private SingletonReleaser
{


    static T *m_inst;

private:
    //only the singleton store should call this method
    void release()
    {
        delete this;
    }
    const char *getType() const
    {
        return typeid(this).name();
    }

protected:
    Singleton(){}
    virtual ~Singleton(){}

public:
    static inline T& inst()
    { 
        if(m_inst == 0)
        {
            m_inst = new T;


            singletonStore.addSingleton(m_inst);
        }

        return *m_inst;
    }
    static inline T* instp()
    { 
        if(m_inst == 0)
        {
            m_inst = new T;

            singletonStore.addSingleton(m_inst);
        }
        return m_inst;
    }
};

template<class T>
T *Singleton<T>::m_inst = 0;




template<class T>
class SingletonNoRegister
{


    static T *m_inst;

protected:
    SingletonNoRegister(){}
    virtual ~SingletonNoRegister(){ }

public:
    static inline T& inst()
    { 
        if(m_inst == 0)
        {
            m_inst = new T;

        }

        return *m_inst;
    }
    static inline T* instp()
    { 
        if(m_inst == 0)
        {
            m_inst = new T;
        }
        return m_inst;
    }
};

template<class T>
T *SingletonNoRegister<T>::m_inst = 0;


#endif // singleton_h__

