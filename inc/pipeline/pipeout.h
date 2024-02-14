#ifndef _KMICKI_PIPELINE_PIPEOUT_H_
#define _KMICKI_PIPELINE_PIPEOUT_H_

#include <memory>
#include <chrono>

namespace kmicki::pipeline
{
    // For sending pipelined object to the next thread in pipeline
    // With triple buffering (T - object's type)
    template<class T>
    class PipeOut
    {
        public:
        // PipeOut needs 3 instances of T.
        // Default constructor calls new T() without arguments
        // to create them.
        PipeOut();

        // This constructor allows to provide 3 instances
        // of T by pointer. PipeOut grabs ownership of them.
        PipeOut(T* inst1, T* inst2, T* inst3);
        ~PipeOut();

        // Methods to be used by current operation:

        // Get object to modify so that is sent to next thread in pipeline.
        // Needs to be obtained before each iteration.
        T & GetDataToFill();
        // Get reference to unique_ptr to object to fill. Needs to be obtained only once.
        std::unique_ptr<T> const& GetPointerToFill();
        // Send the modified object that was obtained earlier with GetDataToFill()
        void SendData();
        // Check if last object was received
        bool WasReceived();

        // Methods to be used by next operation:

        // Wait for object to be sent and receive it.
        T & GetData();
        // Get reference to unique_ptr to received data. Wait for data using WaitForData().
        std::unique_ptr<T> const& GetPointer();
        // Wait for object to be send. 
        // Use together with reference to unique_ptr obtained by GetPointer()
        void WaitForData();
        // Wait for object to be send with timeout. 
        // Use together with reference to unique_ptr obtained by GetPointer().
        // Returns true when data was obtained
        template<class R, class P>
        bool WaitForData(std::chrono::duration<R,P> timeout);

        // Check if data is waiting.
        // Use together with reference to unique_ptr obtained by GetPointer()
        bool TryData();

        // Force the wait to continue.
        void Flush();

        private:
        std::unique_ptr<T> bufMod,bufSent,bufRcv;
        std::mutex bufSentMutex;
        std::condition_variable bufSentConditionVariable;
        bool bufWasSent;
    };
}

#include "impl/pipeout.hpp"

#endif