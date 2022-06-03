#include "hiddev/hiddevreader.h"
#include "hiddev/hiddevfinder.h"
#include "sdgyrodsu/sdhidframe.h"
#include "sdgyrodsu/presenter.h"
#include "cemuhook/cemuhookprotocol.h"
#include "cemuhook/cemuhookserver.h"
#include "sdgyrodsu/cemuhookadapter.h"
#include "log/log.h"
#include <iostream>
#include <future>
#include <thread>

using namespace kmicki::sdgyrodsu;
using namespace kmicki::hiddev;
using namespace kmicki::log;
using namespace kmicki::cemuhook::protocol;
using namespace kmicki::cemuhook;

const LogLevel cLogLevel = LogLevelTrace; // change to Default when configuration is possible
const bool cRunPresenter = false;

const int cFrameLen = 64;
const int cScanPeriodUs = 3850;
const uint16_t cVID = 0x28de;
const uint16_t cPID = 0x1205;
const std::string cVersion = "1.13-NEXT-DEV";

void PresenterRun(HidDevReader * reader)
{
    auto & frameServe = reader->GetServe();
    auto const& data = frameServe.GetPointer();
    int temp;
    void* tempPtr = reinterpret_cast<void*>(&temp);
    Presenter::Initialize();
    while(true)
    {
        auto lock = frameServe.GetConsumeLock();
        Presenter::Present(GetSdFrame(*data));
    }
    Presenter::Finish();
}

int main()
{
    if(cRunPresenter)
        SetLogLevel(LogLevelNone);
    else
        SetLogLevel(cLogLevel);

    { LogF() << "SteamDeckGyroDSU Version: " << cVersion; }
    // Steam Deck controls: usb device VID: 28de, PID: 1205
    int hidno = FindHidDevNo(cVID,cPID);
    if(hidno < 0) 
    {
        Log("Steam Deck Controls' HID device not found.");
        return 0;
    }

    { LogF() << "Found Steam Deck Controls' HID device at /dev/usb/hiddev" << hidno; }
    
    HidDevReader reader(hidno,cFrameLen,cScanPeriodUs);

    reader.SetStartMarker({ 0x01, 0x00, 0x09, 0x40 });

    CemuhookAdapter adapter(reader);
    Server server(adapter);

    uint32_t lastInc = 0;
    int stopping = 0;

    std::unique_ptr<std::thread> presenter;
    if(cRunPresenter)
        presenter.reset(new std::thread(PresenterRun,&reader));

    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}