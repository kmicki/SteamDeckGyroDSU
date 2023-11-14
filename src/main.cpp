#include "hiddev/hiddevreader.h"
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
using namespace kmicki::log::logbase;
using namespace kmicki::cemuhook::protocol;
using namespace kmicki::cemuhook;

const LogLevel cLogLevel = LogLevelDebug; // change to Default when configuration is possible
const bool cRunPresenter = false;
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

const char testRunFlag = 't';
const char helpFlag = 'h';

const std::unordered_map<char,std::string> flagDef =
{
    {testRunFlag,"--testrun"},
    {helpFlag,"--help"}
};

const std::unordered_map<char,std::string> flagDesc =
{
    {testRunFlag,"\tRead from controller without client connected."},
    {helpFlag,"\tThis message."}
};

void HelpMessage()
{
    Log("Usage: " + cExecutableName + " [parameters]");
    Log(" ");
    Log("Parameters:");
    for(const auto& [key,val] : flagDef)
        { LogF() << '-' << key << "," << val << flagDesc.at(key); }
};

void ProcessPars(const int &argc, char **argv, const std::unordered_map<char,std::function<void()>> &flagActions)
{
    if(argc <= 1)
        return;

    for(int i = 1;i < argc;++i)
    {
        std::string arg(argv[i]);

        if(arg.length() < 2 || arg[0] != '-')
            throw std::runtime_error("Unknown parameter: " + arg);

        std::string flagStr = "";

        if(arg[0] == '-' && arg[1] != '-')
            flagStr = arg.substr(1);

        std::size_t fpos;
        bool anyFlag = false;

        for(const auto& [key, val] : flagDef)
        {
            if(flagStr.find(key) != std::string::npos || arg == val)
            {
                (flagActions.at(key))();
                anyFlag = true;
                while((fpos = flagStr.find(key)) != std::string::npos)
                    flagStr.erase(fpos,1);
            }
        }

        if(flagStr.length() > 0)
            throw std::runtime_error("Unknown flags: -" + flagStr);
        if(!anyFlag)
            throw std::runtime_error("Unknown parameter: " + arg);
    }
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
    
    HidDevReader reader(cVID,cPID,cInterfaceNumber,cFrameLen,cScanTimeUs);
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