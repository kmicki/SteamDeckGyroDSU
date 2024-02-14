#ifndef _KMICKI_PIPELINE_THREAD_H_
#define _KMICKI_PIPELINE_THREAD_H_

#include <mutex>
#include <thread>

namespace kmicki::pipeline
{
    // Represents single thread in the pipeline
    class Thread
    {
        public:
        Thread();
        ~Thread();
        // Start the thread.
        void Start();
        // Stop the thread.
        void Stop();
        // Kill the thread.
        void Kill();
        // Restart the thread.
        void Restart();
        // Kill and restart.
        void ForceRestart();
        // Kill if stopping fails.
        template<class R, class P>
        void TryStopThenKill(std::chrono::duration<R,P> timeout);
        void TryStopThenKill();
        // Force restart if normal restart fails.
        template<class R, class P>
        void TryRestartThenForceRestart(std::chrono::duration<R,P> timeout);
        void TryRestartThenForceRestart();
        // Check if the thread is running.
        bool IsStarted();
        // Check if the thread is trying to stop
        bool IsStopping();

        protected:
        // Method that executes on the thread.
        virtual void Execute() = 0;
        // Method used to determine if thread should continue to run.
        // Should be called inside Execute()
        bool ShouldContinue();
        // Force thread to continue through all waits on other pipeline threads
        virtual void FlushPipes() = 0;

        private:
        std::unique_ptr<std::thread> executeThread;
        std::thread::native_handle_type threadHandle;
        std::mutex stopMutex;
        bool stop;

        static const std::chrono::milliseconds cTimeout;
    };
}

#include "impl/thread.hpp"

#endif