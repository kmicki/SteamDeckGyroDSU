#include "hiddev/hiddevreader.h"
#include "log/log.h"

using namespace kmicki::log;

namespace kmicki::hiddev
{
    // Definition - ServeFrame

    HidDevReader::ServeFrame::ServeFrame(PipeOut<frame_t> & _frame) 
    : frame(_frame), frames(), framesMutex(), framesCv()
    { }

    Serve<frame_t> & HidDevReader::ServeFrame::GetServe()
    {
        {
            std::unique_lock lock(framesMutex);
            auto& ptr = frames.emplace_back();
            ptr.reset(new Serve<frame_t>(frame.GetPointer()));
            lock.unlock();
            framesCv.notify_all();
            Log("HidDevReader::ServeFrame: New consumer of frames",LogLevelDebug);
            return *ptr;
        }
    }

    HidDevReader::ServeFrame::~ServeFrame()
    {
        TryStopThenKill();
    }

    void HidDevReader::ServeFrame::StopServe(Serve<frame_t> & serve)
    {
        std::shared_lock shLock(framesMutex);
        for(auto x = frames.begin();x != frames.end();++x)
            if(&(*(*x)) == &serve)
            {
                shLock.unlock();
                std::lock_guard lock(framesMutex);
                frames.erase(x);
                Log("HidDevReader::ServeFrame: Stop serving frames to consumer.",LogLevelDebug);
                return;
            }
    }

    void HidDevReader::ServeFrame::HandleMissedFrames(int &serveCnt, std::vector<int> &missedTicks, std::vector<int> &nonMissedTicks, std::vector<std::string> &serveNames)
    {
        if(GetLogLevel() < LogLevelDebug)
            return;
        static const int cReportMissedTicksPeriod = 250;

        int newSize;
        if(serveCnt != (newSize = frames.size()))
        {
            missedTicks.clear();
            missedTicks.resize(newSize,0);
            nonMissedTicks.clear();
            nonMissedTicks.resize(newSize,0);
            if(newSize < serveCnt)
                serveNames.resize(newSize);
            for(int i = serveCnt; i < newSize; ++i)
            {
                std::stringstream name;
                name << "HidDevReader::ServeFrame::Serve[" << i << "]";
                serveNames.push_back(name.str());
            }
            serveCnt = newSize;
        }

        for(int i=0;i<frames.size();++i)
            HandleMissedTicks(serveNames[i],"frames",frames[i]->WasConsumedNoLock(),missedTicks[i],cReportMissedTicksPeriod,nonMissedTicks[i]);

    }

    void HidDevReader::ServeFrame::Execute()
    {
        Log("HidDevReader::ServeFrame: Started.",LogLevelDebug);

        std::vector<int> missedTicks(0);
        std::vector<int> nonMissedTicks(0);
        std::vector<std::string> serveNames(0);
        int serveCnt = 0;
        
        while(ShouldContinue())
        {
            WaitForServes();
            if(!ShouldContinue())
                break;
            {
                std::lock_guard lock(framesMutex);
                auto locks = GetServeLocks();
                HandleMissedFrames(serveCnt, missedTicks, nonMissedTicks, serveNames);
            
                frame.WaitForData();
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        Log("HidDevReader::ServeFrame: Stopped.",LogLevelDebug);
    }

    void HidDevReader::ServeFrame::FlushPipes()
    {
        frame.SendData();
        framesCv.notify_all();
    }

    void HidDevReader::ServeFrame::WaitForServes()
    {
        std::unique_lock lock(framesMutex);
        framesCv.wait(lock,[&] { return frames.size() > 0 || !ShouldContinue(); });
    }

    std::vector<Serve<frame_t>::ServeLock> HidDevReader::ServeFrame::GetServeLocks() 
    {
        std::vector<Serve<frame_t>::ServeLock> result;
        for(auto & serve : frames)
        {
            result.push_back(std::move(serve->GetServeLock()));
        }
        return std::move(result);
    }
}