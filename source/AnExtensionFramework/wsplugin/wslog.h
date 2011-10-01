#ifndef wslog_h__
#define wslog_h__

#include <string>
#include <map>
#include <set>
#include <algorithm>

#include "../singleton.h"
#include "wsexception.h"
#include "wsthread.h"

#undef DEBUG
#undef INFO
#undef WARN
#undef ERROR
#undef QUIET
#undef MAX
namespace Log
{
    enum Level { QUIET=0, INFO, DEBUG, WARN, ERROR, MAX };
}

class LogHandler
{
public:
    LogHandler(){}
    virtual ~LogHandler(){}
    virtual void Log(Log::Level, const std::string &s) = 0;
};

class NullLogHandler : public LogHandler
{
public:
    NullLogHandler(){}
    virtual ~NullLogHandler(){}
    virtual void Log(Log::Level, const std::string &s) {}
};

class LogFormatter
{
public:
    LogFormatter(){}
    virtual ~LogFormatter(){}
    virtual std::string Format(Log::Level level, const std::string &s) = 0;
};

class DefaultLogFormatter : public LogFormatter
{
public:
    DefaultLogFormatter(){}
    virtual ~DefaultLogFormatter(){}
    virtual std::string Format(Log::Level level, const std::string &s){ return s; }
};

class LogFilter
{
public:
    LogFilter(){}
    virtual ~LogFilter(){}
    virtual bool Filter(Log::Level level, const std::string &s) = 0;
};

class DefaultLogFilter : public LogFilter
{
public:
    DefaultLogFilter(){}
    virtual ~DefaultLogFilter(){}
    virtual bool Filter(Log::Level level, const std::string &s) { return true; }
};

template<class TLogger>
class TLogInterface
{
    CRITICAL_SECTION mutex;
    std::map<Log::Level, std::set<TLogger*> > loggers;
    Log::Level currentLogLevel; // current log level

    struct DeleteLogger
    {
        void operator()(TLogger *l){ delete l; }
    };


    void DeleteLoggerSet(std::set<TLogger*> &l)
    { 
        std::for_each(l.begin(), l.end(), DeleteLogger());
        l.clear();
    }

    struct LogFunc
    {
        const Log::Level lv;
        const std::string &s;

        LogFunc(const Log::Level lv, const std::string &s) : lv(lv), s(s){}

        void operator ()(TLogger* l)
        {
            l->Log(lv, s);
        }
    };

    void LogFuncSet(Log::Level lv, const std::set<TLogger*> &l, const std::string &s)
    {
        std::for_each(l.begin(), l.end(), LogFunc(lv, s));
    }

public:
    TLogInterface() : currentLogLevel(Log::MAX)
    {
        InitializeCriticalSection(&mutex);
        loggers[Log::DEBUG];
        loggers[Log::INFO];
        loggers[Log::WARN];
        loggers[Log::ERROR];
        loggers[Log::QUIET];
    }
    ~TLogInterface()
    {
        ClearAll();
    }

    Log::Level ToLogLevel(unsigned int level) const
    {
        switch(level)
        {
        case Log::DEBUG: return Log::DEBUG; 
        case Log::INFO: return Log::INFO; 
        case Log::WARN: return Log::WARN; 
        case Log::ERROR: return Log::ERROR; 
        case Log::QUIET: return Log::QUIET; 
        case Log::MAX: return Log::MAX; 

        default:
            throw WSException("Failed to convert log level");
        }
    }


    Log::Level GetLogLevel() const
    {
        if(currentLogLevel == Log::MAX)
            return ToLogLevel(currentLogLevel-1);
        return currentLogLevel;
    }

    void SetLogLevel(unsigned int level)
    {
        try
        {
            currentLogLevel = ToLogLevel(level);
        }
        catch(const WSException &)
        {

        }
    }

    void Log(unsigned int level, const std::string &s)
    {
        Lock lock(&mutex);
        Log::Level lv;

        try
        {
            lv = ToLogLevel(level);
            if(level > Log::QUIET && level <= (unsigned int)currentLogLevel)
                LogFuncSet(lv, loggers[lv], s);
        }
        catch(const WSException &)
        {

        }
    }

    void Log(const std::string &s)
    {

        Log(Log::INFO, s);
    }

    void AddLogger(Log::Level level, TLogger *logger)
    {
        Lock lock(&mutex);
        logger->logInterface = this;
        loggers[level].insert(logger);
    }
    void AddLoggerAllLevel(Log::Level level, TLogger *logger)
    {
        AddLogger(Log::DEBUG, logger);
        AddLogger(Log::INFO, logger);
        AddLogger(Log::WARN, logger);
        AddLogger(Log::ERROR, logger);
        AddLogger(Log::QUIET, logger);
    }
    void RemoveLogger(TLogger *logger)
    {
        Lock lock(&mutex);
        if(logger[level].find(logger) != logger[level].end())
        {
            loggers[level].erase(logger);
            delete logger;
        }
    }

    void Clear(Log::Level level)
    {
        Lock lock(&mutex);
        DeleteLoggerSet(loggers[level]);
    }
    void ClearAll()
    {
        Lock lock(&mutex);
        for (std::map<Log::Level, std::set<TLogger*> >::iterator it = loggers.begin();
            it != loggers.end(); ++it)
        {
            DeleteLoggerSet(it->second);
        }
    }


};


class Logger
{
    friend class TLogInterface<Logger>;

private:
    TLogInterface<Logger> *logInterface;
    LogHandler *const logHandler;
    LogFormatter *const formatter;
    LogFilter *const filter;
public:
    Logger(LogHandler *const logHandler = new NullLogHandler, 
        LogFormatter *const formatter = new DefaultLogFormatter, 
        LogFilter *const filter = new DefaultLogFilter) 
        : logHandler(logHandler), formatter(formatter), filter(filter)
    {

    }
    void Log(Log::Level level, const std::string &s)
    {
        if(filter->Filter(level, s))
            logHandler->Log(level, formatter->Format(level, s));
    }
    ~Logger()
    {
        delete logHandler;
        delete formatter;
        delete filter;
    }
};

typedef TLogInterface<Logger> LogInterface;


class LogFactory : public Singleton<LogFactory>
{
    friend class Singleton<LogFactory>;
    
    LogInterface log;

    LogFactory()
    {

    }

public:
    LogInterface *GetLogInterface()
    {
        return &log;
    }

};



#endif // wslog_h__

