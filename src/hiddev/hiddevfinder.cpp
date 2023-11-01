#include "hiddev/hiddevfinder.h"
#include "shell/shell.h"
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <systemd/sd-device.h>
    
using namespace kmicki::shell;

namespace kmicki::hiddev
{
    const std::string cHiddevPath = "/dev/usb/";
    const std::string cHiddevPrefix = std::string("hiddev");
    const std::string cSubsystem = "usb";
    const std::string cDevType = "usb_device";
    const std::string cProductPropertyName = "PRODUCT";

    inline std::string GetHexStringId(uint16_t id)
    {
        return std::move(
            ((
                std::ostringstream()) << std::setfill('0') 
                                      << std::setw(4) 
                                      << std::nouppercase 
                                      << std::hex 
                                      << id).str());
    }

    // Find N of the HID device matching provided vendor ID and product ID.
    // N being the number in path: /dev/usb/hiddevN
    int FindHidDevNo(uint16_t vid, uint16_t pid)
    {
        auto vidStr = GetHexStringId(vid);
        auto pidStr = GetHexStringId(pid);

        // Loop through all /dev/usb/hiddev* files
        for (const auto & hiddevFile : std::filesystem::directory_iterator(cHiddevPath))
        {
            // Get hiddev* device
            sd_device *hidDevice = NULL;
            if(sd_device_new_from_devname(&hidDevice, hiddevFile.path().c_str()) != 0)
                continue;

            // Go up to usb_device
            sd_device *usbDevice = NULL;
            if(sd_device_get_parent_with_subsystem_devtype(hidDevice,cSubsystem.c_str(),cDevType.c_str(),&usbDevice) != 0)
                continue;

            // Check vid,pid
            const char *product = NULL;
            if(sd_device_get_property_value(usbDevice,cProductPropertyName.c_str(),&product) != 0)
                continue;
            if(vidStr == std::string(product).substr(0,4) && pidStr == std::string(product).substr(5,4))
            {
                std::string fName = hiddevFile.path().filename();
                if(fName.length() > cHiddevPrefix.length())
                {
                    std::string strNum = fName.substr(cHiddevPrefix.length());
                    std::stringstream ss(strNum);
                    int i;
                    if(!(ss >> i).fail() && (ss >> std::ws).eof())
                        return i; // found matching hiddev file
                }
            }
        }

        return -1;
    }
}