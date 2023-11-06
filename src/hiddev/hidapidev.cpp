#include "log/log.h"
#include "hiddev/hidapidev.h"
#include <iostream>
#include <iomanip>

namespace kmicki::hiddev
{
    int HidApiDev::hidApiInitialized = 0;

    HidApiDev::HidApiDev(const uint16_t& _vId, const uint16_t _pId, const int& _interfaceNumber, int readTimeoutUs)
        : vId(_vId),pId(_pId),dev(nullptr),timeout(readTimeoutUs), interfaceNumber(_interfaceNumber)
    { 
        if(hidApiInitialized == 0)
        {
            if(hid_init() < 0)
                throw std::runtime_error("Error: HIDAPI initialization failed.");
        }
        ++hidApiInitialized;
    }

    HidApiDev::~HidApiDev()
    {
        Close();

        --hidApiInitialized;
        if(hidApiInitialized <= 0)
        {
            hidApiInitialized = 0;
            hid_exit();
        }
    }

    bool HidApiDev::Open()
    {
        if(dev != nullptr)
            Close();

        auto info = hid_enumerate(vId,pId);

        while(info != nullptr)
        {
            if(info->interface_number == interfaceNumber)
            {
                dev = hid_open_path(info->path);
                if(dev != nullptr)
                {
                    hid_set_nonblocking(dev,0);
                    return true;
                }
            }

            info = info->next;
        }

        return false;
    }

    bool HidApiDev::Close()
    {
        if(dev != nullptr)
            hid_close(dev);
        dev = nullptr;
        return true;
    }

    bool HidApiDev::IsOpen()
    {
        return dev != nullptr;
    }

    int HidApiDev::Read(std::vector<char> & data)
    {
        if(dev == nullptr)
            return 0;

        int readCnt = 0;
        
        do {
            auto readCntLoc = hid_read_timeout(dev,(unsigned char*)(data.data()+readCnt),data.size()-readCnt,timeout);
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