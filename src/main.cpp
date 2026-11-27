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
#include <cstdlib>
#include <fstream>
#include <hidapi/hidapi.h>

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

const std::string cVersion = "2.2";   // Release version

// Default mapping of IIO IMU axes onto Steam Deck frame axes
// (accel/gyro: RightToLeft, TopToBottom, FrontToBack).
// Verified on ROG Xbox Ally X (bmi323) with gravity in 3 poses:
//   Deck frame axes point to: right side (+x), top edge (-y), out of the screen (-z).
// Accel and gyro must use the same mapping and it must be a proper rotation (even number of sign flips),
// otherwise gyro and accelerometer disagree and sensor fusion in clients drifts.
// May be overridden with environment variables SDGYRO_IIO_ACCEL_MAP and SDGYRO_IIO_GYRO_MAP (e.g. "x,-y,-z").
const std::string cIioDefaultAccelMap = "x,-y,-z";
const std::string cIioDefaultGyroMap = "x,-y,-z";

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

std::string ReadSysFile(std::string const& path)
{
    std::ifstream f(path);
    std::string value;
    std::getline(f,value);
    return value;
}

bool IsSteamDeck()
{
    auto product = ReadSysFile("/sys/class/dmi/id/product_name");
    return product == "Jupiter" || product == "Galileo";
}

bool SteamDeckControlsPresent()
{
    bool found = false;
    auto info = hid_enumerate(cVID,cPID);
    for(auto i = info; i != nullptr; i = i->next)
        if(i->interface_number == cInterfaceNumber)
            found = true;
    hid_free_enumeration(info);
    return found;
}

void SetIioAxisMap(char const* envName, std::string const& defaultMap, std::array<IioAxisMap,3> &map)
{
    auto env = std::getenv(envName);
    if(env != nullptr)
    {
        if(ParseIioAxisMap(env,map))
        {
            { LogF() << "Using " << envName << "=" << env; }
            return;
        }
        { LogF() << "Invalid value of " << envName << ": " << env << ". Using default."; }
    }
    ParseIioAxisMap(defaultMap,map);
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

    std::unique_ptr<HidDevReader> readerPtr;

    if(cUseHiddevFile)
    {
        int hidno = FindHidDevNo(cVID,cPID);
        if(hidno < 0) 
        {
            Log("Steam Deck Controls' HID device not found.");
            return 0;
        }

        { LogF() << "Found Steam Deck Controls' HID device at /dev/usb/hiddev" << hidno; }
        
        readerPtr.reset(new HidDevReader(hidno,cFrameLen,cScanTimeUs));
    }
    else
    {
        std::string iioName;
        std::string iioPath;
        if(!IsSteamDeck() && !SteamDeckControlsPresent())
            iioPath = FindIioImu(iioName);

        if(iioPath.empty())
        {
            Log("Using Steam Deck Controls' HID device.");
            readerPtr.reset(new HidDevReader(cVID,cPID,cInterfaceNumber,cFrameLen,cScanTimeUs));
        }
        else
        {
            { LogF() << "Device: " << ReadSysFile("/sys/class/dmi/id/product_name") << ". Using IIO IMU device '" << iioName << "' at " << iioPath << "."; }
            IioImuConfig config;
            config.devicePath = iioPath;
            SetIioAxisMap("SDGYRO_IIO_ACCEL_MAP",cIioDefaultAccelMap,config.accel);
            SetIioAxisMap("SDGYRO_IIO_GYRO_MAP",cIioDefaultGyroMap,config.gyro);
            readerPtr.reset(new HidDevReader(config,cFrameLen,cScanTimeUs));
        }
    }

    HidDevReader &reader = *readerPtr;

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