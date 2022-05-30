#include "hiddev/hiddevreader.h"
#include "log/log.h"

using namespace kmicki::log;

namespace kmicki::hiddev
{
    // Definition - AnalyzeMissedFrames
    HidDevReader::AnalyzeMissedFrames::AnalyzeMissedFrames(PipeOut<int64_t> & _diff, 
                                                           Metronome & _metronome, 
                                                           SignalOut & _readStuck, 
                                                           SignalOut & _unsynced)
    : diff(_diff), readStuck(_readStuck), unsynced(_unsynced),
      ScanTime(), metronome(_metronome)
    { 
    }

    void HidDevReader::AnalyzeMissedFrames::Execute()
    {
        // Constants
        static const int cMaxLossPeriod = 10000;
        static const int cMaxStuckPeriod = 2000;
        static const std::chrono::seconds lossAnalysisMinWindow(1);     // Minimum time between frame losses to execute loss analysis
        static const int cReadStuckHold = 10;
        static const int cBigLossThreshold = 25;
        static const int cMaxLossAccepted = 1000;

        auto const& diffData = diff.GetPointer();
        auto const& scanPeriod = ScanTime.GetPointerToFill();
        
        std::chrono::microseconds scanPeriodCurrent = metronome.GetCurrentScanTime();

        { LogF() << "HidDevReader::AnalyzeMissedFrames: Current scan time: " << scanPeriodCurrent.count() << "us."; }

        metronome.SetScanTimeIn(ScanTime);
        
        int lossPeriod;
        int noStuckPeriod;
        bool lossAnalysis;
        bool stuckAnalysis;
        std::chrono::system_clock::time_point lastAnalyzedFrameLossTime;
        bool passedNoLossAnalysis;
        int readStuckLeft;

        lossPeriod = 0;         // number of packets since last loss of frames
        noStuckPeriod = 0;
        lossAnalysis = false;   // loss analysis is active
        stuckAnalysis = false;
        lastAnalyzedFrameLossTime = std::chrono::system_clock::now();
        passedNoLossAnalysis = false; // Flag that minimum window between lost frames has passed in order to execute loss analysis
        readStuckLeft = 0;

        Log("HidDevReader::AnalyzeMissedFrames: Started.");

        while(ShouldContinue())
        {
            diff.WaitForData();
            if(!ShouldContinue())
                break;
            
            bool checkUnsynced = unsynced.TrySignal();
            if(readStuck.TrySignal() || checkUnsynced)
            {
                readStuckLeft =  cReadStuckHold;
                Log("HidDevReader::AnalyzeMissedFrames: Read stuck start monitoring.");
            }

            if(readStuckLeft > 0)
            {
                --readStuckLeft;
                if (*diffData > 1)
                {
                    if(stuckAnalysis || *diffData > 2)
                    {
                        { LogF() << "HidDevReader::AnalyzeMissedFrames: Frame loss after reading restart: " << *diffData-1 << " frames. Previous " << noStuckPeriod << " frames were read without restart."; }
                        auto update = std::chrono::microseconds(std::min(200,std::max(1,(6000-noStuckPeriod)/2000)));
                        scanPeriodCurrent += update;
                        *scanPeriod = scanPeriodCurrent;
                        { LogF() << "HidDevReader::AnalyzeMissedFrames: Request scan period update by " << update.count() << "us to " << scanPeriodCurrent.count() << "us."; }
                        ScanTime.SendData();
                    }
                    else if(!stuckAnalysis)
                    {
                        { LogF() << "HidDevReader::AnalyzeMissedFrames: Frame loss after reading restart: " << *diffData-1 << " frames. Activate reading stuck monitoring."; }
                        stuckAnalysis = true;
                    }
                        
                    readStuckLeft = 0;
                    noStuckPeriod = 0;
                    lossPeriod = 0;
                    lossAnalysis = false;
                    continue;
                }
                if(readStuckLeft <= 0)
                {
                    Log("HidDevReader::AnalyzeMissedFrames: Read stuck monitoring stopped.");
                }
            }
            else if(stuckAnalysis)
            {
                    if(++noStuckPeriod > cMaxStuckPeriod)
                    {
                        { LogF() << "HidDevReader: Over " << cMaxStuckPeriod << " frames since last reading stuck."; }
                        noStuckPeriod = 0;
                        stuckAnalysis = false;
                    }
            }
            
            if(passedNoLossAnalysis || std::chrono::system_clock::now()-lastAnalyzedFrameLossTime > lossAnalysisMinWindow)
            {
                passedNoLossAnalysis = true;

                bool result = false;
                if(lossAnalysis)
                {
                    if(++lossPeriod > cMaxLossPeriod)
                    {
                        { LogF() << "HidDevReader: Over " << cMaxLossPeriod << " frames since last frame loss."; }
                        lossPeriod = 0;
                        lossAnalysis = false;
                            
                    }
                }
                if(*diffData > 1 && *diffData < cMaxLossAccepted)
                {
                    if(*diffData > cBigLossThreshold)
                    {
                        if(lossAnalysis)
                        {
                            { LogF() << "HidDevReader::AnalyzeMissedFrames: High loss: " << *diffData-1 << " frames after " << lossPeriod << " frames without loss."; };
                            auto update = std::chrono::microseconds(std::min(200,std::max(1,(6000-lossPeriod)/400)));
                            scanPeriodCurrent -= update;
                            *scanPeriod = scanPeriodCurrent;
                            { LogF() << "HidDevReader::AnalyzeMissedFrames: Request scan period update by -" << update.count() << "us to " << scanPeriodCurrent.count() << "us."; }
                            ScanTime.SendData();
                        }
                        else
                            { LogF() << "HidDevReader::AnalyzeMissedFrames: High loss: " << *diffData-1 << " frames. Activate loss monitoring."; };
                        lossAnalysis = true;
                        result = true;
                    }
                    else if(!lossAnalysis)
                    {
                        { LogF() << "HidDevReader::AnalyzeMissedFrames: Low loss: " << *diffData-1 << " frames. Activate loss monitoring"; }
                        lossAnalysis = true;
                    }
                    else
                    {
                        { LogF () << "HidDevReader::AnalyzeMissedFrames: Low loss: " << *diffData-1 << " frames after " << lossPeriod << " frames without loss."; };
                        ++scanPeriodCurrent;
                        *scanPeriod = scanPeriodCurrent;
                        { LogF () << "HidDevReader::AnalyzeMissedFrames: Request scan period update by 1us to " << scanPeriodCurrent.count() << "us."; }
                        ScanTime.SendData();
                        result = true;
                    }
                    lossPeriod = 0;
                }

                if(result)
                {
                    lastAnalyzedFrameLossTime = std::chrono::system_clock::now();
                    passedNoLossAnalysis = false;
                }
            }
        }

        metronome.UnsetScanTimeIn();

        Log("HidDevReader::AnalyzeMissedFrames: Stopped.");
    }

    void HidDevReader::AnalyzeMissedFrames::FlushPipes()
    {
        diff.SendData();
    }
}