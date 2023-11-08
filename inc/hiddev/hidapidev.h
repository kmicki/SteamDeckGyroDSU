#ifndef _KMICKI_HIDDEV_HIDAPIDEV_
#define _KMICKI_HIDDEV_HIDAPIDEV_

#include <stdint.h>
#include <vector>
#include <hidapi/hidapi.h>

namespace kmicki::hiddev
{
    class HidApiDev
    {
        public:
        HidApiDev() = delete;
        HidApiDev(const uint16_t& _vId, const uint16_t _pId, const int& _interfaceNumber, int readTimeoutUs);
        ~HidApiDev();

        bool Open();
        int Read(std::vector<char> & data);
        bool Close();
        bool IsOpen();

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