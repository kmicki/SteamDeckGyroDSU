#include "hiddev/hiddevreader.h"
#include "hiddev/iioreader.h"
#include "hiddev/hiddevfinder.h"
#include "hiddev/hidapidev.h"
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
const bool cUseHiddevFile = false;
const bool cTestRun = false;

const int cFrameLen = 64;       // Steam Deck Controls' custom HID report length in bytes
const int cScanTimeUs = 4000;   // Steam Deck Controls' period between received report data in microseconds
const uint16_t cVID = 0x28de;   // Steam Deck Controls' USB Vendor-ID
const uint16_t cPID = 0x1205;   // Steam Deck Controls' USB Product-ID
const int cInterfaceNumber = 2; // Steam Deck Controls' USB Interface Number

const std::string cVersion = "2.1";   // Release version

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

void PresenterRun(MotionReader * reader)
{
    reader->Start();
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

    std::unique_ptr<MotionReader> readerPtr;

    bool useIio = false;

    if(cUseHiddevFile)
    {
        int hidno = FindHidDevNo(cVID,cPID);
        if(hidno < 0) 
        {
            Log("Steam Deck Controls' HID device not found. Trying IIO...");
            useIio = true;
        }
        else
        {
            { LogF() << "Found Steam Deck Controls' HID device at /dev/usb/hiddev" << hidno; }
            readerPtr.reset(new HidDevReader(hidno,cFrameLen,cScanTimeUs));
        }
    }
    else
    {
        if (HidApiDev::Exists(cVID,cPID,cInterfaceNumber))
        {
            Log("Found Steam Deck Controls' HID device (HIDAPI).");
            readerPtr.reset(new HidDevReader(cVID,cPID,cInterfaceNumber,cFrameLen,cScanTimeUs));
        }
        else
        {
            Log("Steam Deck Controls' HID device not found (HIDAPI). Trying IIO...");
            useIio = true;
        }
    }

    if (useIio)
    {
        readerPtr.reset(new IioReader(cScanTimeUs));
    }

    MotionReader &reader = *readerPtr;

    reader.SetStartMarker({ 0x01, 0x00, 0x09, 0x40 }); // Beginning of every Steam Decks' HID frame

    CemuhookAdapter adapter(reader);
    reader.SetNoGyro(adapter.NoGyro);
    Server server(adapter);

    uint32_t lastInc = 0;
    int stopping = 0;

    std::unique_ptr<std::thread> presenter;
    if(cRunPresenter)
        presenter.reset(new std::thread(PresenterRun,&reader));

    if(cTestRun && !cRunPresenter)
        reader.Start();

    {
        std::unique_lock lock(stopMutex);
        stopCV.wait(lock,[]{ return stop; });
    }

    Log("SteamDeckGyroDSU exiting.");

    return 0;
}