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

    namespace logbase
    {
        void Log(std::string message,LogLevel type)
        {
            Log("",message,type);
        }
        
        void Log(std::string prefix,std::string message,LogLevel type)
        {
            if(type > currentLogType)
                return;

            static std::mutex logMutex;
            std::lock_guard lock(logMutex);
            std::cout << prefix << message << std::endl;
        }
        
        LogF::LogF(std::string prefix, LogLevel type)
        : std::ostringstream(), logType(type), logPrefix(prefix), moved(false)
        {};

        LogF::LogF(LogLevel type)
        : LogF("",type)
        {};

        LogF::LogF(LogF&& other)
        : std::ostringstream(std::move(other)), logType(other.logType), logPrefix(other.logPrefix), moved(false)
        {
            other.moved = true;
        }

        LogF::~LogF()
        {
            if(moved) return;
            Log(logPrefix,str(),logType);
        }

        void LogF::LogNow()
        {
            Log(logPrefix,str(),logType);
            str("");
        }
    }
}