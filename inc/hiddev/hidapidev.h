#ifndef _KMICKI_HIDDEV_HIDAPIDEV_
#define _KMICKI_HIDDEV_HIDAPIDEV_

#include <stdint.h>
#include <vector>
#include <hidapi/hidapi.h>

#include "hiddev.h"

namespace kmicki::hiddev
{
    class HidApiDev
    {
        public:
        HidApiDev() = delete;
        HidApiDev(const uint16_t& _vId, const uint16_t _pId, const int& _interfaceNumber, int readTimeoutUs);
        ~HidApiDev();

        bool Open();
        int Read(frame_t & data);
        bool Close();
        bool IsOpen();
        bool EnableGyro();
        bool Write(frame_t & data);

        private:
        uint16_t vId;
        uint16_t pId;
        int interfaceNumber;
        hid_device *dev;
        static int hidApiInitialized;
        int timeout;
    };
}

#endif