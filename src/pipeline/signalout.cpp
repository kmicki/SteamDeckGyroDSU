#include "pipeline/signalout.h"

namespace kmicki::pipeline
{
    // Definition - SignalOut

    SignalOut::SignalOut()
    : signalMutex(),signalConditionVariable(),signal(false)
    { }

    SignalOut::~SignalOut()
    { }

    void SignalOut::SendSignal()
    {
        {
            std::lock_guard lock(signalMutex);
            signal = true;
        }
        signalConditionVariable.notify_all();
    }

    bool SignalOut::WasReceived()
    {
        std::lock_guard lock(signalMutex);
        return !signal;
    }

    void SignalOut::WaitForSignal()
    {
        std::unique_lock lock(signalMutex);
        signalConditionVariable.wait(lock,[&] { return signal; });
        signal = false;
    }

    bool SignalOut::TrySignal()
    {
        std::lock_guard lock(signalMutex);
        if(signal)
        {
            signal = false;
            return true;
        }
        return false;
    }

    void SignalOut::Flush()
    {
        SendSignal();
    }
}