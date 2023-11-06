#include "hiddev/hiddevreader.h"
#include "log/log.h"

#include <sstream>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    // Constants
    const int HidDevReader::cInputRecordLen = 8;    // Number of bytes that are read from hiddev file per 1 byte of HID data.
    const int HidDevReader::cByteposInput = 4;      // Position in the raw hiddev record (of INPUT_RECORD_LEN length) where 
                                                    // HID data byte is.

    void HandleMissedTicks(std::string name, std::string tickName, bool received, int & ticks, int period, int & nonMissed)
    {
        if(GetLogLevel() < LogLevelDebug)
            return;
        if(!received)
        {
            if(ticks == 1)
                { LogF(LogLevelDebug) << name << ": Start missing " << tickName << "."; }
            ++ticks;
            if(ticks % period == 0)
            {
                { LogF(LogLevelDebug) << name << ": Missed " << period << " " << tickName << " after " << nonMissed << " " << tickName << ". Still being missed."; }
                if(ticks > period)
                    ticks -= period;
            }
        }
        else if(ticks > period)
        {
            { LogF(LogLevelDebug) << name << ": Missed " << ((ticks+1) % period - 1) << " " << tickName << ". Not being missed anymore."; }
            ticks = 0;
            nonMissed = 0;
        }
        else if(ticks > 0)
        {
            { LogF(LogLevelDebug) << name << ": Missed " << ticks << " " << tickName << " after " << nonMissed << " " << tickName << "."; }
            ticks = 0;
            nonMissed = 0;
        }
        else
            ++nonMissed;
    }

    // Definition - HidDevReader

    void HidDevReader::AddOperation(Thread * operation)
    {
        pipeline.emplace_back(operation);
    }

    void HidDevReader::ConstructPipeline(ReadData *_readData, int const& _frameLen, int const& scanTimeUs, bool useProcessData)
    {
        auto* readDataOp = _readData;
        ProcessData* processData;
        ServeFrame* serveFrame;
        if(useProcessData)
        {
            processData = new ProcessData(_frameLen, *readDataOp, scanTimeUs);
            serveFrame = new ServeFrame(processData->Frame);
        }
        else
            serveFrame = new ServeFrame(readDataOp->Data);

        AddOperation(readDataOp);
        if(useProcessData)
            AddOperation(processData);
        AddOperation(serveFrame);

        serve = serveFrame;
        readData = readDataOp;

        Log("HidDevReader: Pipeline initialized. Waiting for start...",LogLevelDebug);
    }

    HidDevReader::HidDevReader(int const& hidNo, int const& _frameLen, int const& scanTimeUs) 
    : frameLen(_frameLen), startStopMutex()
    {
        if(hidNo < 0) throw std::invalid_argument("hidNo");

        std::stringstream inputFilePathFormatter;
        inputFilePathFormatter << "/dev/usb/hiddev" << hidNo;
        inputFilePath = inputFilePathFormatter.str();

        auto* readDataOp = new ReadDataFile(inputFilePath, _frameLen, scanTimeUs);
        ConstructPipeline(readDataOp, _frameLen, scanTimeUs);
    }


    HidDevReader::HidDevReader(uint16_t const& vId, uint16_t const& pId, int const& interfaceNumber ,int const& _frameLen, int const& scanTimeUs) 
    : frameLen(_frameLen), startStopMutex()
    {
        auto* readDataOp = new ReadDataApi(vId, pId, interfaceNumber, _frameLen, scanTimeUs);
        ConstructPipeline(readDataOp, _frameLen, scanTimeUs,false);
    }


    HidDevReader::~HidDevReader()
    {
        Stop();
    }

    Serve<HidDevReader::frame_t> & HidDevReader::GetServe()
    {
        return serve->GetServe();
    }

    void HidDevReader::StopServe(Serve<frame_t> & _serve)
    {
        serve->StopServe(_serve);
    }

    void HidDevReader::SetStartMarker(std::vector<char> const& marker)
    {
        if(readData == nullptr)
            return;
        readData->SetStartMarker(marker);
    }

    void HidDevReader::Start()
    {
        std::lock_guard startLock(startStopMutex); // prevent starting and stopping at the same time

        Log("HidDevReader: Attempting to start the pipeline...",LogLevelDebug);

        for (auto& thread : pipeline)
            thread->Start();

        Log("HidDevReader: Started the pipeline.");
    }
    
    void HidDevReader::Stop()
    {
        std::lock_guard startLock(startStopMutex); // prevent starting and stopping at the same time

        Log("HidDevReader: Attempting to stop the pipeline...",LogLevelDebug);

        for (auto thread = pipeline.rbegin(); thread != pipeline.rend(); ++thread)
            (*thread)->TryStopThenKill(std::chrono::seconds(10));

        Log("HidDevReader: Stopped the pipeline.");
    }

    bool HidDevReader::IsStarted()
    {
        for (auto& thread : pipeline)
            if(thread->IsStarted())
                return true;
        
        return false;
    }

    bool HidDevReader::IsStopping()
    {
        if(!IsStarted())
            return false;
        
        for (auto& thread : pipeline)
            if(!thread->IsStarted() || thread->IsStopping())
                return true;

        return false;
    }
}