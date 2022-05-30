#include "hiddev/hiddevreader.h"
#include "log/log.h"

using namespace kmicki::log;

namespace kmicki::hiddev
{    
    // Definition - Metronome
    HidDevReader::Metronome::Metronome(std::chrono::microseconds _scanTime)
    : scanTime(_scanTime),initialScanTime(_scanTime),scanTimeIn(nullptr),Tick(),scanTimeInPtr(nullptr)
    { }

    HidDevReader::Metronome::~Metronome()
    { }

    void HidDevReader::Metronome::SetScanTimeIn(PipeOut<std::chrono::microseconds> & _scanTimeIn)
    {
        scanTimeIn = &_scanTimeIn;
        scanTimeInPtr = &(scanTimeIn->GetPointer());
    }

    void HidDevReader::Metronome::UnsetScanTimeIn()
    {
        scanTimeIn = nullptr;
        scanTimeInPtr = nullptr;
    }

    std::chrono::microseconds HidDevReader::Metronome::GetInitialScanTime()
    {
        std::shared_lock lock(scanTimeMutex);
        return initialScanTime;
    }

    std::chrono::microseconds HidDevReader::Metronome::GetCurrentScanTime()
    {
        std::shared_lock lock(scanTimeMutex);
        return scanTime;
    }

    void HandleMissedTicks(std::string name, std::string tickName, bool received, int & ticks, int period, int & nonMissed)
    {
        if(!received)
        {
            if(ticks == 1)
                { LogF() << name << ": Start missing " << tickName << "."; }
            ++ticks;
            if(ticks % period == 0)
            {
                { LogF() << name << ": Missed " << period << " " << tickName << " after " << nonMissed << " " << tickName << ". Still being missed."; }
                if(ticks > period)
                    ticks -= period;
            }
        }
        else if(ticks > period)
        {
            { LogF() << name << ": Missed " << ((ticks+1) % period - 1) << " " << tickName << ". Not being missed anymore."; }
            ticks = 0;
            nonMissed = 0;
        }
        else if(ticks > 0)
        {
            { LogF() << name << ": Missed " << ticks << " " << tickName << " after " << nonMissed << " " << tickName << "."; }
            ticks = 0;
            nonMissed = 0;
        }
        else
            ++nonMissed;
    }

    void HidDevReader::Metronome::Execute()
    {
        static const int cReportMissedTicksPeriod = 250;
        int missedTicks = 0;
        int nonMissedTicks = 0;

        auto time = scanTime;

        Log("HidDevReader::Metronome: Started.");

        while(ShouldContinue())
        {
            if(TryGetScanTime())
            {
                std::lock_guard lock(scanTimeMutex);
                time = scanTime;
            }

            std::this_thread::sleep_for(time);

            HandleMissedTicks("HidDevReader::Metronome","ticks",Tick.WasReceived(),missedTicks,cReportMissedTicksPeriod,nonMissedTicks);

            Tick.SendSignal();
        }

        scanTimeInPtr = nullptr;

        Log("HidDevReader::Metronome: Stopped.");
    }

    void HidDevReader::Metronome::FlushPipes()
    {
        if(scanTimeIn != nullptr)
            scanTimeIn->SendData();
    }

    bool HidDevReader::Metronome::TryGetScanTime()
    {
        static const std::chrono::microseconds cMinimum(3700);
        static const std::chrono::microseconds cMaximum(4000);

        if(scanTimeIn == nullptr || !scanTimeIn->TryData())
            return false;

        if(**scanTimeInPtr < cMinimum || **scanTimeInPtr > cMaximum)
        {
            { LogF() << "HidDevReader::Metronome: Incoming scan period outside limits : " << (*scanTimeInPtr)->count() << " us."; }
            return false;
        }

        std::lock_guard lock(scanTimeMutex);
        scanTime = **scanTimeInPtr;
        { LogF() << "HidDevReader::Metronome: Changed scan period to : " << scanTime.count() << " us."; }

        return true;
    }
}