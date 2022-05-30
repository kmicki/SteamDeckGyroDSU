#include "hiddev/hiddevreader.h"
#include "log/log.h"

#include <sstream>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    // Constants
    const int HidDevReader::cInputRecordLen = 8;   // Number of bytes that are read from hiddev file per 1 byte of HID data.

    // Definition - HidDevReader

    void HidDevReader::AddOperation(Thread * operation)
    {
        pipeline.emplace_back(operation);
    }

    HidDevReader::HidDevReader(int hidNo, int _frameLen, int scanTime) 
    : frameLen(_frameLen), scanPeriod(scanTime), startStopMutex()
    {
        if(hidNo < 0) throw std::invalid_argument("hidNo");

        std::stringstream inputFilePathFormatter;
        inputFilePathFormatter << "/dev/usb/hiddev" << hidNo;
        inputFilePath = inputFilePathFormatter.str();

        auto* metronome = new Metronome(std::chrono::microseconds(scanTime));
        auto* readData = new ReadData(inputFilePath, _frameLen, metronome->Tick);
        auto* processData = new ProcessData(_frameLen, *readData);
        auto* serveFrame = new ServeFrame(processData->Frame);
        auto* analyzer = new AnalyzeMissedFrames(
                                processData->Diff,
                                *metronome,
                                processData->ReadStuck,
                                readData->Unsynced);

        AddOperation(metronome);
        AddOperation(readData);
        AddOperation(processData);
        AddOperation(serveFrame);
        AddOperation(analyzer);

        serve = serveFrame;

        Log("HidDevReader: Pipeline initialized. Waiting for start...");
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

    void HidDevReader::Start()
    {
        std::lock_guard startLock(startStopMutex); // prevent starting and stopping at the same time

        Log("HidDevReader: Attempting to start the pipeline...");

        for (auto& thread : pipeline)
            thread->Start();

        Log("HidDevReader: Started the pipeline.");
    }
    
    void HidDevReader::Stop()
    {
        std::lock_guard startLock(startStopMutex); // prevent starting and stopping at the same time

        Log("HidDevReader: Attempting to stop the pipeline...");

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