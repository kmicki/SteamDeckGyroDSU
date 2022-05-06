#include "hiddevreader.h"
#include "hiddevfinder.h"
#include "sdhidframe.h"
#include "presenter.h"
#include "cemuhookprotocol.h"
#include "cemuhookserver.h"
#include "cemuhookadapter.h"
#include <iostream>
#include <future>

using namespace kmicki::sdgyrodsu;
using namespace kmicki::hiddev;

bool showIncrement = false;

#define FRAME_LEN 64
#define SCAN_PERIOD_US 3900

#define VERSION "1.6"

#define VID 0x28de
#define PID 0x1205

typedef HidDevReader::frame_t frame_t;

using namespace kmicki::sdgyrodsu;
using namespace kmicki::cemuhook::protocol;
using namespace kmicki::cemuhook;

static std::exception_ptr teptr = nullptr;

static bool stop = false;

void WaitForKey()
{
    std::cin.get();
    stop = true;
}

int main()
{
    std::cout << "Version: " << VERSION << std::endl;
    // Steam Deck controls: usb device VID: 28de, PID: 1205
    int hidno = FindHidDevNo(VID,PID);
    if(hidno < 0) 
    {
        std::cerr << "Device not found." << std::endl;
        return 0;
    }

    std::cout << "Found hiddev" << hidno << std::endl;
    
    HidDevReader reader(hidno,FRAME_LEN,SCAN_PERIOD_US);
    CemuhookAdapter adapter(reader);
    Server server(adapter);

    uint32_t lastInc = 0;
    int stopping = 0;

    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        uint32_t const& newInc = *reinterpret_cast<uint32_t const*>(reader.Frame().data()+4);
        if(lastInc > 0 && newInc == lastInc)
        {
            if(reader.IsStarted() || stopping > 5)
            {
                std::cout << "Framegrab is stuck. Aborting." << std::endl;
                std::abort();
            }
            if(reader.IsStopping())
                ++stopping;
        }
        if(!reader.IsStarted() && !reader.IsStopping())
            lastInc = 0;
        else
            lastInc = newInc;
        if(!reader.IsStopping())
            stopping = 0;
    }

    return 0;
}