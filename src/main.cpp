#include "hiddevreader.h"
#include "hiddevfinder.h"
#include "sdhidframe.h"
#include "shell.h"
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

void drawData(frame_t frame,int num)
{
    static int maxSpan = 0;
    static uint32_t lastInc;
    uint32_t newInc = *(reinterpret_cast<uint32_t *>(frame.data()+4));

    if(lastInc && newInc-lastInc > maxSpan)
        maxSpan = newInc-lastInc;

    move(0,0);
    printw("%5d %d %d             ",num,newInc-lastInc,maxSpan);
    move(2,0);
    int k = 2;
    for (int i = 0; i < FRAME_LEN; ++i) {
        printw("%.2x ", (unsigned char) frame[i]);
        if(i % 16 == 15)
            move(++k,0);
    }

    SdHidFrame* sd = reinterpret_cast<SdHidFrame*>(frame.data());

    ++k;
    move(++k,0); printw("U1: %d         ",sd->Unknown1);
    move(++k,0); printw("U2: %d         ",sd->Unknown2);
    move(++k,0); printw("U3: %d         ",sd->Unknown3);
    move(++k,0); printw("U4: %d         ",sd->Unknown4);
    ++k;
    move(++k,0); printw("A_RL: %d         ",sd->AccelAxisRightToLeft);
    move(++k,0); printw("A_TB: %d         ",sd->AccelAxisTopToBottom);
    move(++k,0); printw("A_FB: %d         ",sd->AccelAxisFrontToBack);
    ++k;
    move(++k,0); printw("G_RL: %d         ",sd->GyroAxisRightToLeft);
    move(++k,0); printw("G_TB: %d         ",sd->GyroAxisTopToBottom);
    move(++k,0); printw("G_FB: %d         ",sd->GyroAxisFrontToBack);

    refresh();

    lastInc = newInc;
}

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


    initscr();
    refresh();

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
            drawData(frame,i);
            reader.UnlockFrame();
            ++i;
        }
        output.close();
        ++part;
    }
    return 0;
}