#include "log/log.h"
#include <iostream>
#include <mutex>

namespace kmicki::log
{
    static LogLevel currentLogType = LogLevelDefault;

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
    : std::stringstream(), logType(type)
    {};

    LogF::~LogF()
    {
        Log(str(),logType);
    }

    void LogF::LogNow()
    {
        Log(str(),logType);
        std::stringstream newStream;
        swap(newStream);
    }
}