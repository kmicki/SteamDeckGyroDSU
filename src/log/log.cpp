#include "log/log.h"
#include <iostream>
#include <mutex>

namespace kmicki::log
{
    void Log(std::string message)
    {
        static std::mutex logMutex;
        std::lock_guard lock(logMutex);
        std::cout << message << std::endl;
    }

    LogF::LogF()
    : std::stringstream()
    {};

    LogF::~LogF()
    {
        Log(str());
    }

    void LogF::LogNow()
    {
        Log(str());
        std::stringstream newStream;
        swap(newStream);
    }
}