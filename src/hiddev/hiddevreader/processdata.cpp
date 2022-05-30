#include "hiddev/hiddevreader.h"
#include "log/log.h"

using namespace kmicki::log;

namespace kmicki::hiddev
{
    // Definition - ProcessData

    HidDevReader::ProcessData::ProcessData(int const& _frameLen, ReadData & _data)
    : readData(_data), data(_data.Data), ReadStuck(), Diff(), 
      Frame(new frame_t(_frameLen),new frame_t(_frameLen),new frame_t(_frameLen))
    { }

    HidDevReader::ProcessData::~ProcessData()
    { }

    void HidDevReader::ProcessData::Execute()
    {
        static const std::chrono::microseconds cReadDataRestartTimeout(500);
        static const int cReportMissedTicksPeriod = 250;
        int missedTicks = 0;
        int nonMissedTicks = 0;

        // Constants
        static const int cByteposInput = 4;     // Position in the raw hiddev record (of INPUT_RECORD_LEN length) where 
                                                // HID data byte is.

        // ReadData sometimes hangs on reading from hiddev file forever. This has to be detected and thread needs to be reset forcefully.
        static const std::chrono::milliseconds readTaskTimeout(6);      // Timeout of ReadData - means that reading from hiddev file hangs
        
        auto const& frame = Frame.GetPointerToFill();
        auto const& hidData = data.GetPointer();
        
        uint32_t lastInc = 0;

        auto const& diff = Diff.GetPointerToFill();

        int missedLossTicks = 0;
        int nonMissedLossTicks = 0;

        Log("HidDevReader::ProcessData: Started.");

        while(ShouldContinue())
        {
            if(!data.WaitForData(readTaskTimeout))
            {
                Log("HidDevReader::ProcessData: Reading from hiddev file stuck. Force-restarting reading task.");
                ReadStuck.SendSignal();
                readData.TryRestartThenForceRestart(cReadDataRestartTimeout);
                continue;
            }
            if(!ShouldContinue())
                break;

            // Each byte is encapsulated in a record
            for (int i = 0, j = cByteposInput; i < frame->size(); ++i,j+=cInputRecordLen) {
                (*frame)[i] = (*hidData)[j];
            }

            uint32_t const& newInc = *reinterpret_cast<uint32_t const*>(frame->data()+4);
            if(lastInc != 0)
            {
                *diff = (int64_t) newInc - (int64_t) lastInc;

                HandleMissedTicks("HidDevReader::ProcessData","loss reports",Diff.WasReceived(),missedLossTicks,cReportMissedTicksPeriod,nonMissedTicks);
                
                Diff.SendData();
            }
            lastInc = newInc;
            
            HandleMissedTicks("HidDevReader::ProcessData","frames",Frame.WasReceived(),missedTicks,cReportMissedTicksPeriod,nonMissedLossTicks);

            Frame.SendData();
        }
        
        Log("HidDevReader::ProcessData: Stopped.");
    }

    void HidDevReader::ProcessData::FlushPipes()
    {
        data.SendData();
    }
}