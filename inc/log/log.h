#ifndef _KMICKI_LOG_LOG_H_
#define _KMICKI_LOG_LOG_H_

#include <string>
#include <sstream>

namespace kmicki::log
{
        // Log a string message
        void Log(std::string message);

        // class for logging formatted message
        // Behaves like output stream and message gets logged on destruction.
        // Usage: { LogF msg; msg << "This is an example message number " << nr << "!"; }
        class LogF : public std::stringstream
        {
            public:

            LogF();
            ~LogF();

            // Log message without desctruction.
            void LogNow();
        };
}

#endif