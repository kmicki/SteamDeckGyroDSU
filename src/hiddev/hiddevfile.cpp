// for ppoll()
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "hiddev/hiddevfile.h"
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

namespace kmicki::hiddev
{
    static const int cUsToTimeout = 1000;

    HidDevFile::HidDevFile(std::string const& _filePath, int readTimeoutUs, bool const& open)
        : filePath(_filePath), fileDescriptors{{-1,POLLIN,0}}, 
        timeout{0,readTimeoutUs*cUsToTimeout}, file(fileDescriptors[0].fd)
    {
        if(open)
            Open();
    }

    bool HidDevFile::Open()
    {
        file = open(filePath.c_str(),O_RDONLY);
        return file >= 0;
    }

    bool HidDevFile::Close()
    {
        auto result = close(file);
        file = -1;
        return result == 0;
    }

    bool HidDevFile::IsOpen()
    {
        return file >= 0 && (fcntl(file, F_GETFD) != -1 || errno != EBADF);
    }

    int HidDevFile::Read(std::vector<char> & data)
    {
        if(file < 0)
            return 0;
        
        auto retval = ppoll(fileDescriptors,1,&timeout,nullptr);
        
        if(retval == 0)
            return 0;

        if(retval < 0)
            return retval;

        int readCnt = 0;
            
        do {
            auto readCntLoc = read(file, data.data()+readCnt,data.size()-readCnt);
            if(readCntLoc < 0)
                return readCntLoc;
            if(readCntLoc == 0)
                return (readCnt==0)?-1:readCnt;
            readCnt += readCntLoc;
        }
        while(readCnt < data.size());

        return readCnt;
    }
}