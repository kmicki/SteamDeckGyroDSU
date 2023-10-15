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
#include <csignal>

using namespace kmicki::sdgyrodsu;
using namespace kmicki::hiddev;
using namespace kmicki::log;
using namespace kmicki::cemuhook::protocol;
using namespace kmicki::cemuhook;

const LogLevel cLogLevel = LogLevelDebug; // change to Default when configuration is possible
const bool cRunPresenter = false;

const int cFrameLen = 64;       // Steam Deck Controls' custom HID report length in bytes
const int cScanTimeUs = 4000;   // Steam Deck Controls' period between received report data in microseconds
const uint16_t cVID = 0x28de;   // Steam Deck Controls' USB Vendor-ID
const uint16_t cPID = 0x1205;   // Steam Deck Controls' USB Product-ID

const std::string cVersion = "1.15";   // Release version

bool stop = false;
std::mutex stopMutex = std::mutex();
std::condition_variable stopCV = std::condition_variable();

void SignalHandler(int signal)
{
    {
        LogF msg;
        msg << "Incoming signal: ";
        bool stopCmd = true;
        switch(signal)
        {
            case SIGINT:
                msg << "SIGINT";
                break;
            case SIGTERM:
                msg << "SIGTERM";
                break;
            default:
                msg << "Other";
                stopCmd = false;
                break;
        }
        if(!stopCmd)
        {
            msg << ". Unhandled, ignoring...";
            return;
        }
        msg << ". Stopping...";
    }

    {
        std::lock_guard lock(stopMutex);
        stop = true;
    }
    stopCV.notify_all();
}

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
    signal(SIGINT,SignalHandler);
    signal(SIGTERM,SignalHandler);

    stop = false;

    if(cRunPresenter)
        SetLogLevel(LogLevelNone);
    else
        SetLogLevel(cLogLevel);

    { LogF() << "SteamDeckGyroDSU Version: " << cVersion; }

    int hidno = FindHidDevNo(cVID,cPID);
    if(hidno < 0) 
    {
        Log("Steam Deck Controls' HID device not found.");
        return 0;
    }

    { LogF() << "Found Steam Deck Controls' HID device at /dev/usb/hiddev" << hidno; }
    
    HidDevReader reader(hidno,cFrameLen,cScanTimeUs);

    reader.SetStartMarker({ 0x01, 0x00, 0x09, 0x40 }); // Beginning of every Steam Decks' HID frame

    CemuhookAdapter adapter(reader);
    Server server(adapter);

    uint32_t lastInc = 0;
    int stopping = 0;

    std::unique_ptr<std::thread> presenter;
    if(cRunPresenter)
        presenter.reset(new std::thread(PresenterRun,&reader));

    {
        std::unique_lock lock(stopMutex);
        stopCV.wait(lock,[]{ return stop; });
    }

    Log("SteamDeckGyroDSU exiting.");

    return 0;
}