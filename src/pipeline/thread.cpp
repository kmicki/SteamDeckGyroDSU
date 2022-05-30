#include "pipeline/thread.h"

namespace kmicki::pipeline
{
    // Definition - Thread

    const std::chrono::milliseconds Thread::cTimeout(100);

    Thread::Thread()
    : executeThread(),stopMutex(),stop(false)
    {}
    
    Thread::~Thread()
    {
        if(executeThread != nullptr)
            TryStopThenKill();
    }

    void Thread::Start()
    {
        if(executeThread != nullptr)
            return;
        
        stop = false;
        executeThread.reset(new std::thread(&Thread::Execute,this));
        threadHandle = executeThread->native_handle();
    }

    void Thread::Stop()
    {
        if(executeThread == nullptr)
            return;

        {
            std::lock_guard lock(stopMutex);
            stop = true;
        }
        FlushPipes();
        executeThread->join();
        executeThread.reset();
        stop = false;
    }

    void Thread::Kill()
    {
        if(executeThread == nullptr)
            return;
        pthread_cancel(threadHandle);
        executeThread->join();
        executeThread.reset();
        stop = false;
    }

    void Thread::Restart()
    {
        Stop();
        Start();
    }

    void Thread::ForceRestart()
    {
        Kill();
        Start();
    }

    void Thread::TryStopThenKill()
    {
        TryStopThenKill(cTimeout);
    }

    void Thread::TryRestartThenForceRestart()
    {
        TryRestartThenForceRestart(cTimeout);
    }

    bool Thread::IsStarted()
    {
        return executeThread != nullptr;
    }

    bool Thread::IsStopping()
    {
        if(!IsStarted())
            return false;
        std::lock_guard lock(stopMutex);
        return stop;
    }

    bool Thread::ShouldContinue()
    {
        std::lock_guard lock(stopMutex);
        return !stop;
    }
}