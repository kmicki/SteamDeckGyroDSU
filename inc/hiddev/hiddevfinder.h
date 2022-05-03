#ifndef _KMICKI_HIDDEV_HIDDEVFINDER_H_
#define _KMICKI_HIDDEV_HIDDEVFINDER_H_

#include <cstdint>

namespace kmicki::hiddev
{
    // find which X among /dev/usb/hiddevX fits provided VID+PID
    int FindHidDevNo(uint16_t vid, uint16_t pid);
}

#endif
