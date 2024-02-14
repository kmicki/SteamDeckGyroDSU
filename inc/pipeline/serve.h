#ifndef _KMICKI_PIPELINE_SERVE_H_
#define _KMICKI_PIPELINE_SERVE_H_

#include <mutex>
#include <condition_variable>

namespace kmicki::pipeline
{
    // Serve object of type T to a single client
    template<class T>
    class Serve
    {
        public:

        class ConsumeLock
        {
            public:
            ConsumeLock() = delete;
            ConsumeLock(std::mutex & _mutex,std::condition_variable & _cv, bool & _served);
            ConsumeLock(ConsumeLock&& other);
            ~ConsumeLock();

            private:
            std::unique_lock<std::mutex> lock;
            bool & served;
            bool moved;
        };

        Serve();
        Serve(std::unique_ptr<T> const& _object);
        ~Serve();
        void SetObject(std::unique_ptr<T> const& _object);
        bool IsObjectSet();

        bool WasConsumed();
        // Use following if lock was acquired already by creating ServeLock
        bool WasConsumedNoLock();
        
        // Get pointer to data being served.
        // Use together with WaitForData()
        std::unique_ptr<T> const& GetPointer();

        ConsumeLock GetConsumeLock();

        // Force the wait to continue.
        void Flush();

        class ServeLock
        {
            public:
            ServeLock() = delete;
            ServeLock(std::mutex & _mutex,std::condition_variable & _cv, bool & _served);
            ServeLock(ServeLock&& other);
            ~ServeLock();

            private:
            std::condition_variable & cv;
            std::unique_lock<std::mutex> lock;
            bool & served;
            bool moved;
        };

        ServeLock GetServeLock();

        private:

        std::unique_ptr<T> const* object;
        std::mutex serveMutex;
        std::condition_variable serveConditionVariable;
        bool served;
    };
}

#include "impl/serve.hpp"

#endif