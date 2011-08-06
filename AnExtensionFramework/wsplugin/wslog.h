#ifndef wslog_h__
#define wslog_h__

#include <string>
#include <iostream>
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
#undef FATAL
#undef MAX
namespace Log
{
    enum Level { DEBUG=0, INFO, WARN, ERROR, FATAL, MAX };
}

class LogHandler
{
public:
    LogHandler(){}
    virtual ~LogHandler(){}
    virtual void Log(const std::string &s) = 0;
};

class NullLogHandler : public LogHandler
{
public:
    NullLogHandler(){}
    virtual ~NullLogHandler(){}
    virtual void Log(const std::string &s) {}
};

class FileLogHandler : public LogHandler
{
    std::ostream &f;
public:
    virtual ~FileLogHandler(){}
    FileLogHandler(std::ostream &f) : f(f)
    {

    }
    virtual void Log(const std::string &s)
    {
        f << s << std::endl;
    }
};

class ConsoleLogHandler : public FileLogHandler
{
public:
    virtual ~ConsoleLogHandler(){}
    ConsoleLogHandler() : FileLogHandler(std::cout) {}
};

class ConsoleErrorLogHandler : public FileLogHandler
{
public:
    virtual ~ConsoleErrorLogHandler(){}
    ConsoleErrorLogHandler() : FileLogHandler(std::cerr) {}
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
    Log::Level logLevel; // logLevel is an index to the loggers array

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
        const std::string &s;

        LogFunc(const std::string &s) : s(s){}

        void operator ()(TLogger* l)
        {
            l->Log(s);
        }
    };

    void LogFuncSet(const std::set<TLogger*> &l, const std::string &s)
    {
        std::for_each(l.begin(), l.end(), LogFunc(s));
    }

    Log::Level ToLogLevel(unsigned int level) const
    {
        switch(level)
        {
            case Log::DEBUG: return Log::DEBUG; 
            case Log::INFO: return Log::INFO; 
            case Log::WARN: return Log::WARN; 
            case Log::ERROR: return Log::ERROR; 
            case Log::FATAL: return Log::FATAL; 
            case Log::MAX: return Log::MAX; 

            default:
                throw WSException("Failed to convert log level");
        }
    }

public:
    TLogInterface()
    {
        InitializeCriticalSection(&mutex);
        loggers[Log::DEBUG];
        loggers[Log::INFO];
        loggers[Log::WARN];
        loggers[Log::ERROR];
        loggers[Log::FATAL];
    }
    ~TLogInterface()
    {
        ClearAll();
    }

    Log::Level GetLogLevel() const
    {
        return logLevel;
    }

    void SetLogLevel(unsigned int level)
    {
        try
        {
            logLevel = ToLogLevel(level);
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
            LogFuncSet(loggers[lv], s);
        }
        catch(const WSException &)
        {

        }
    }

    void Log(const std::string &s)
    {

        Log(logLevel, s);
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
        AddLogger(Log::FATAL, logger);
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
    void Log(const std::string &s)
    {
        if(filter->Filter(logInterface->GetLogLevel(), s))
            logHandler->Log(formatter->Format(logInterface->GetLogLevel(), s));
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

    LogFactory()
    {

    }

public:
    void SetupConsoleLog(LogInterface &logInterface)
    {
        logInterface.AddLogger(Log::DEBUG, new Logger(new ConsoleLogHandler));
        logInterface.AddLogger(Log::INFO, new Logger(new ConsoleLogHandler));
        logInterface.AddLogger(Log::WARN, new Logger(new ConsoleLogHandler));
        logInterface.AddLogger(Log::ERROR, new Logger(new ConsoleErrorLogHandler));
        logInterface.AddLogger(Log::FATAL, new Logger(new ConsoleErrorLogHandler));
    }

};



#endif // wslog_h__

