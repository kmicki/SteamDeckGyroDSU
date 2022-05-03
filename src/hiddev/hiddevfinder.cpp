#include "hiddevfinder.h"
#include "shell.h"
#include <sstream>
#include <iomanip>
    
using namespace kmicki::shell;

namespace kmicki::hiddev
{

    int FindHidDevNo(uint16_t vid, uint16_t pid)
    {
        std::string output;
        int bus,dev,bus2,dev2;
        int testlen;

        std::ostringstream cmd;
        cmd << "lsusb | grep -i '";
        cmd << std::setw(4) << std::setfill('0') << std::setbase(16) << vid << ":" ;
        cmd << std::setw(4) << std::setfill('0') << std::setbase(16) << pid;
        cmd << "' | sed -e \"s/.*Bus \\([0-9]\\+\\) Device \\([0-9]\\+\\).*/\\1 \\2/g\"";
        
        if(ExecuteCommand(cmd.str(),output) || (testlen = output.length()) != 8)
            return -1;

        std::istringstream str(output);
        str >> bus >> dev;
        if (bus == 0 || dev == 0)
            return -1;

        for (int i = 0; i < 10; i++)    
        {
            std::ostringstream ostr;
            ostr << "udevadm info --query=all /dev/usb/hiddev" << i << " | grep 'P:' | sed -e \"s/.*usb\\([0-9]\\+\\).*\\.\\([0-9]\\+\\).*/\\1 \\2/g\"";
            std::string cmd = ostr.str();
            if(ExecuteCommand(cmd,output) || output.length() > 8 ||  output.length() < 4)
                continue;
            std::istringstream str(output);
            str >> bus2 >> dev2;
            if (bus == bus2 || dev == dev2)
                return i;
        }
        return -1;
    }

}