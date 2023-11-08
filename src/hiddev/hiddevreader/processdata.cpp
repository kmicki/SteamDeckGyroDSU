#include "hiddev/hiddevreader.h"
#include "log/log.h"

using namespace kmicki::log;

namespace kmicki::hiddev
{
    static const int cApiScanTimeToTimeout = 3;

    HidDevReader::ProcessData::ProcessData(int const& _frameLen, ReadData & _data, int const& scanTimeUs)
    : readData(_data), data(_data.Data), ReadStuck(), timeout(cApiScanTimeToTimeout*scanTimeUs),
      Frame(new frame_t(_frameLen),new frame_t(_frameLen),new frame_t(_frameLen))
    { }

    HidDevReader::ProcessData::~ProcessData()
    {
        TryStopThenKill();
    }

    void HidDevReader::ProcessData::Execute()
    {
        static const std::chrono::microseconds cReadDataRestartTimeout(500);
        static const int cReportMissedTicksPeriod = 250;
        int missedTicks = 0;
        int nonMissedTicks = 0;
        
        auto const& frame = Frame.GetPointerToFill();
        auto const& hidData = data.GetPointer();

        int missedLossTicks = 0;
        int nonMissedLossTicks = 0;

        Log("HidDevReader::ProcessData: Started.",LogLevelDebug);

        while(ShouldContinue())
        {
            if(!data.WaitForData(timeout))
            {
                Log("HidDevReader::ProcessData: Reading from hiddev file stuck. Force-restarting reading task.",LogLevelDebug);
                ReadStuck.SendSignal();
                readData.TryRestartThenForceRestart(cReadDataRestartTimeout);
                continue;
            }
            if(!ShouldContinue())
                break;

            // Each byte is encapsulated in a record
            for (int i = 0, j = cByteposInput; i < frame->size(); ++i,j+=cInputRecordLen) 
            {
                (*frame)[i] = (*hidData)[j];
            }
            
            HandleMissedTicks("HidDevReader::ProcessData","frames",Frame.WasReceived(),missedTicks,cReportMissedTicksPeriod,nonMissedLossTicks);

            Frame.SendData();
        }
        
        Log("HidDevReader::ProcessData: Stopped.",LogLevelDebug);
    }

    void HidDevReader::ProcessData::FlushPipes()
    {
        data.SendData();
    }
}