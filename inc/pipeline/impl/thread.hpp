#include "../thread.h"
#include <future>

namespace kmicki::pipeline
{
    template<class R, class P>
    void Thread::TryStopThenKill(std::chrono::duration<R,P> timeout)
    {
        auto future = std::async(std::launch::async,&Thread::Stop,this);
        if(future.wait_for(timeout) == std::future_status::timeout)
        {
            pthread_cancel(threadHandle);
            future.wait();
        }
    }

    template<class R, class P>
    void Thread::TryRestartThenForceRestart(std::chrono::duration<R,P> timeout)
    {
        TryStopThenKill(timeout);
        Start();
    }
}