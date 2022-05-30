#ifndef _KMICKI_PIPELINE_SIGNALOUT_H_
#define _KMICKI_PIPELINE_SIGNALOUT_H_

#include <mutex>
#include <condition_variable>

namespace kmicki::pipeline
{
    // For sending signal to the next thread in pipeline
    class SignalOut
    {
        public:
        SignalOut();
        ~SignalOut();

        // Methods to be used by next operation:

        // Send signal
        void SendSignal();
        // Check if last signal was received
        bool WasReceived();

        // Methods to be used by next operation:

        // Wait until signal is sent.
        void WaitForSignal();
        // Check if signal was sent. If not return false.
        bool TrySignal();

        // Force the wait to continue.
        void Flush();

        private:
        std::mutex signalMutex;
        std::condition_variable signalConditionVariable;
        bool signal;
    };

}

#endif