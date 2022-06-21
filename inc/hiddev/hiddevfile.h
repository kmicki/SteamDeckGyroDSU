#ifndef _KMICKI_HIDDEV_HIDDEVFILE_
#define _KMICKI_HIDDEV_HIDDEVFILE_

#include <string>
#include <vector>
#include <sys/select.h>
#include "poll.h"

namespace kmicki::hiddev
{
    class HidDevFile
    {
        public:
        HidDevFile() = delete;
        HidDevFile(std::string const& _filePath, int readTimeoutUs, bool const& open = true);

        bool Open();
        int Read(std::vector<char> & data);
        bool Close();
        bool IsOpen();

        private:
        pollfd fileDescriptors[1];
        int & file;
        std::string filePath;
        timespec timeout;
    };
}

#endif