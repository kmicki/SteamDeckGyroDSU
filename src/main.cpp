#include "hiddevreader.h"
#include "hiddevfinder.h"
#include "sdhidframe.h"
#include "presenter.h"
#include <iostream>
#include <future>

using namespace kmicki::sdgyrodsu;
using namespace kmicki::hiddev;

#define FRAME_LEN 64
#define FRAMECNT_PER_FILE 5000
#define SCAN_PERIOD_MS 4

#define VID 0x28de
#define PID 0x1205

typedef HidDevReader::frame_t frame_t;

using namespace kmicki::sdgyrodsu;

static std::exception_ptr teptr = nullptr;

int main()
{
    // Steam Deck controls: usb device VID: 28de, PID: 1205
    int hidno = FindHidDevNo(VID,PID);
    if(hidno < 0) 
    {
        std::cerr << "Device not found." << std::endl;
        return 0;
    }

    std::cout << "Found hiddev" << hidno << std::endl;
    
    HidDevReader reader(hidno,FRAME_LEN,SCAN_PERIOD_MS);

    Presenter::Initialize();

    std::basic_istream<char, std::char_traits<char>>::int_type (std::istream::*pointerToPeek)() = &std::istream::peek;

    // Set up any key listener
    auto anyKeyListener = std::async(std::launch::async,pointerToPeek,&std::cin);


    while(anyKeyListener.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout) {
        auto const& frame = GetSdFrame(reader.GetNewFrame());
        Presenter::Present(frame);
        reader.UnlockFrame();
    }

    Presenter::Finish();

    return 0;
}