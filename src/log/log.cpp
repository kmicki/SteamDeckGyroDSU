#include "log/log.h"
#include <iostream>
#include <mutex>

namespace kmicki::log
{
    LogLevel currentLogType = LogLevelDefault;

    void SetLogLevel(LogLevel type)
    {
        currentLogType = type;
    }
    
    LogLevel const& GetLogLevel()
    {
        return currentLogType;
    }

    void Log(std::string message,LogLevel type)
    {
        if(type > currentLogType)
            return;

        static std::mutex logMutex;
        std::lock_guard lock(logMutex);
        std::cout << message << std::endl;
    }

    LogF::LogF(LogLevel type)
    : std::ostringstream(), logType(type)
    {};

    LogF::~LogF()
    {
        Log(str(),logType);
    }

    void LogF::LogNow()
    {
        Log(str(),logType);
        std::ostringstream newStream;
        swap(newStream);
    }
}