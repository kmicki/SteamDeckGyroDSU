#ifndef _KMICKI_LOG_LOG_H_
#define _KMICKI_LOG_LOG_H_

#include <string>
#include <sstream>

namespace kmicki::log
{
    enum LogLevel
    {
        LogLevelNone     =   0,
        LogLevelDefault  =   1,
        LogLevelDebug    =   2,
        LogLevelTrace    =   3
    };

    void SetLogLevel(LogLevel type);

    LogLevel const& GetLogLevel();

    // Log a string message
    void Log(std::string message,LogLevel type = LogLevelDefault);

    // class for logging formatted message
    // Behaves like output stream and message gets logged on destruction.
    // Usage: { LogF() << "This is an example message number " << nr << "!"; }
    class LogF : public std::stringstream
    {
        public:

        LogF(LogLevel type = LogLevelDefault);
        ~LogF();

        // Log message without desctruction.
        void LogNow();

        private:
        LogLevel logType;
    };
}

#endif