#include "../pch.h"
#include "../hookah.h"
#include "../wsplugin/wspluginmanager.h"
#include "extension.h"



AXFExtension::AXFExtension()
{

}

AXFExtension::~AXFExtension()
{

}

namespace
{
    void AddEvent_Extender(const char *name)
    {
        PluginManager::inst().GetEvents()[name];
    }

    void FireEvent_Extender(const char *name, void *data)
    {
        PluginManager::inst().FireEvent(name, data);
    }

    void AddExtension_Extender(const char *name, const ExtensionFactory *fac)
    {
        PluginManager::inst().GetExtensionFactories()[name] = *fac;
    }


    class LogHandlerExt : public LogHandler
    {
        LogOutputFunc output;
    public:
        LogHandlerExt(LogOutputFunc output) : output(output){}
        virtual ~LogHandlerExt(){}

        virtual void Log(Log::Level level, const std::string &s)
        {
            output((LogLevel)level, s.c_str());
        }
    };

    class LogFormatterExt : public LogFormatter
    {
        LogFormatterFunc formatter;

    public:
        LogFormatterExt(LogFormatterFunc formatter) : formatter(formatter){}
        virtual ~LogFormatterExt(){}
        virtual std::string Format(Log::Level level, const std::string &s)
        {
            String *outputStr=0;

            formatter(&outputStr, (LogLevel)level, s.c_str());

            if(outputStr)
            {
                std::string output(outputStr->buffer);

                if(outputStr->len > 0 && outputStr->buffer)
                    free(outputStr->buffer);

                return output;
            }
            else
                return std::string();

        }
    };

    class LogFilterExt : public LogFilter
    {
        LogFilterFunc filter;
    public:
        LogFilterExt(LogFilterFunc filter) : filter(filter){}
        virtual ~LogFilterExt(){}
        virtual bool Filter(Log::Level level, const std::string &s)
        {
            if(filter((LogLevel)level, s.c_str()) == WSTRUE)
                return true;
            else
                return false;
        }
    };


    void AddLogger_Extender(LogLevel level, LogOutputFunc logOutputFunc, LogFormatterFunc logFormatterFunc, LogFilterFunc logFilterFunc)
    {
        LogHandler *logHandler;
        LogFormatter *logFormatter;
        LogFilter *logFilter;


        if(logOutputFunc)
            logHandler = new LogHandlerExt(logOutputFunc);
        else
            logHandler = new NullLogHandler;

        if(logFormatterFunc)
            logFormatter = new LogFormatterExt(logFormatterFunc);
        else
            logFormatter = new DefaultLogFormatter;

        if(logFilterFunc)
            logFilter = new LogFilterExt(logFilterFunc);
        else
            logFilter = new DefaultLogFilter;

        Logger *logger = new Logger(logHandler, logFormatter, logFilter);

        LogInterface &logInterface = *LogFactory::inst().GetLogInterface();
        try
        {
            logInterface.AddLogger(logInterface.ToLogLevel((unsigned int)level), logger);
        }
        catch(const std::exception &ex)
        {
            logInterface.Log(Log::ERROR, ex.what());
            delete logger;
        }
        
    }
}


void AXFExtension::SetupExtenderInterface( ExtenderInterface &extender )
{
    extender.data = new ExtenderInterfaceData;
    extender.event = new EventExtenderInterface;
    extender.extension = new ExtensionExtenderInterface;
    extender.log = new LogExtenderInterface;

    extender.event->AddEvent = &AddEvent_Extender;
    extender.event->FireEvent = &FireEvent_Extender;

    extender.extension->AddExtension = &AddExtension_Extender;

    extender.log->AddLogger = &AddLogger_Extender;
}

