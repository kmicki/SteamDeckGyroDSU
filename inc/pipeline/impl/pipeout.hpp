#include "../pipeout.h"

namespace kmicki::pipeline
{

    // Definition PipeOut

    template<class T>
    PipeOut<T>::PipeOut()
    : PipeOut(new T(),new T(),new T())
    { }

    template<class T>
    PipeOut<T>::PipeOut(T* inst1, T* inst2, T* inst3)
    : bufMod(inst1),bufSent(inst2),bufRcv(inst3),
      bufSentMutex(), bufSentConditionVariable(),
      bufWasSent(false)
    { }

    template<class T>
    PipeOut<T>::~PipeOut()
    { }

    template<class T>
    T & PipeOut<T>::GetDataToFill()
    {
        return *bufMod;
    }

    template<class T>
    std::unique_ptr<T> const& PipeOut<T>::GetPointerToFill()
    {
        return bufMod;
    }

    template<class T>
    void PipeOut<T>::SendData()
    {
        {
            std::lock_guard lock(bufSentMutex);
            bufSent.swap(bufMod);
            bufWasSent = true;
        }
        bufSentConditionVariable.notify_all();
    }

    template<class T>
    bool PipeOut<T>::WasReceived()
    {
        std::lock_guard lock(bufSentMutex);
        return !bufWasSent;
    }

    template<class T>
    T & PipeOut<T>::GetData() 
    {
        WaitForData();
        return *bufRcv;
    }

    template<class T>
    std::unique_ptr<T> const& PipeOut<T>::GetPointer()
    {
        return bufRcv;
    }

    template<class T>
    void PipeOut<T>::WaitForData()
    {
        std::unique_lock lock(bufSentMutex);
        bufSentConditionVariable.wait(lock,[&] { return bufWasSent; });
        bufSent.swap(bufRcv);
        bufWasSent = false;
    }

    template<class T>
    template<class R,class P>
    bool PipeOut<T>::WaitForData(std::chrono::duration<R,P> timeout)
    {
        std::unique_lock lock(bufSentMutex);
        if(bufSentConditionVariable.wait_for(lock,timeout,[&](){ return bufWasSent; }))
        {
            bufSent.swap(bufRcv);
            bufWasSent = false;
            return true;
        }
        return false;
    }

    template<class T>
    bool PipeOut<T>::TryData()
    {
        std::lock_guard lock(bufSentMutex);
        if(bufWasSent)
        {
            bufSent.swap(bufRcv);
            bufWasSent = false;
            return true;
        }
        return false;
    }

    template<class T>
    void PipeOut<T>::Flush()
    {
        {
            std::lock_guard lock(bufSentMutex);
            bufWasSent = true;
        }
        bufSentConditionVariable.notify_all();
    }

}