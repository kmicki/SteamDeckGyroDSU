#include "hiddev/hiddevreader.h"
#include "log/log.h"

#include <sstream>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    namespace hdr {
    static const std::string cLogPrefix = "HidDevReader: ";
        #include "log/locallog.h"
    }

    void HandleMissedTicks(std::string prefix, std::string tickName, bool received, int & ticks, int period, int & nonMissed)
    {
        if(GetLogLevel() < LogLevelDebug)
            return;
        if(!received)
        {
            if(ticks == 1)
                { logbase::LogF(LogLevelDebug) << prefix << "Start missing " << tickName << "."; }
            ++ticks;
            if(ticks % period == 0)
            {
                { logbase::LogF(LogLevelDebug) << prefix << "Missed " << period << " " << tickName << " after " << nonMissed << " " << tickName << ". Still being missed."; }
                if(ticks > period)
                    ticks -= period;
            }
        }
        else if(ticks > period)
        {
            { logbase::LogF(LogLevelDebug) << prefix << "Missed " << ((ticks+1) % period - 1) << " " << tickName << ". Not being missed anymore."; }
            ticks = 0;
            nonMissed = 0;
        }
        else if(ticks > 0)
        {
            { logbase::LogF(LogLevelDebug) << prefix << "Missed " << ticks << " " << tickName << " after " << nonMissed << " " << tickName << "."; }
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

    HidDevReader::HidDevReader(uint16_t const& vId, uint16_t const& pId, int const& interfaceNumber, int const& _frameLen, int const& scanTimeUs) 
    : startStopMutex()
    {
        readWriteData = new ReadWriteData(vId,pId,interfaceNumber,frameLen,scanTimeUs)
        serveFrame = new ServeFrame(readData->Data);

        AddOperation(readData);
        AddOperation(serveFrame);

        hdr::Log("Pipeline initialized. Waiting for start...",LogLevelDebug);
    }


    HidDevReader::~HidDevReader()
    {
        Stop();
    }

    Serve<HidDevReader::frame_t> & HidDevReader::GetServe()
    {
        return serve->GetServe();
    }

    void HidDevReader::StopServe(Serve<frame_t> & serve)
    {
        serve->StopServe(serve);
    }

    void HidDevReader::Start()
    {
        std::lock_guard startLock(startStopMutex); // prevent starting and stopping at the same time

        hdr::Log("Attempting to start the pipeline...",LogLevelDebug);

        for (auto& thread : pipeline)
            thread->Start();

        hdr::Log("Started the pipeline.");
    }
    
    void HidDevReader::Stop()
    {
        std::lock_guard startLock(startStopMutex); // prevent starting and stopping at the same time

        hdr::Log("Attempting to stop the pipeline...",LogLevelDebug);

        for (auto thread = pipeline.rbegin(); thread != pipeline.rend(); ++thread)
            (*thread)->TryStopThenKill(std::chrono::seconds(10));

        hdr::Log("Stopped the pipeline.");
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

    void HidDevReader::SetWriteData(SignalOut &writeData)
    {
        if(readDataApi)
            readDataApi->SetNoGyro(_noGyro);
    }
}