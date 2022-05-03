#include "hiddevreader.h"
#include "hiddevfinder.h"
#include "sdhidframe.h"
#include "shell.h"
#include "presenter.h"
#include <ncurses.h>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace kmicki::shell;
using namespace kmicki::sdgyrodsu;
using namespace kmicki::hiddev;

#define FRAME_LEN 64
#define FRAMECNT_PER_FILE 5000
#define SCAN_PERIOD_MS 2

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

    std::cout << "Found hiddev" << hidno << std::endl << "Press any key";
    std::cin.get();
    
    HidDevReader reader(hidno,FRAME_LEN,SCAN_PERIOD_MS);

    Presenter::Initialize();

    int part = 0;

    while(true) {
        std::ostringstream strStr;
        strStr << "dump" << std::setfill('0') << std::setw(3) << part << ".hex";

        std::ofstream output( strStr.str(), std::ios::binary );

        int i = 0;

        while(i < FRAMECNT_PER_FILE)
        {
            auto& frame = reader.GetNewFrame();
            output.write(frame.data(),frame.size());
            Presenter::Present(frame);
            reader.UnlockFrame();
            ++i;
        }
        output.close();
        ++part;
    }

    Presenter::Finish();

    return 0;
}