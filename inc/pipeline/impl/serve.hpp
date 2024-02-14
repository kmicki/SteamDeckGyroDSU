#include "../serve.h"

namespace kmicki::pipeline
{
    // Definition of ConsumeLock

    template<class T>
    Serve<T>::ConsumeLock::ConsumeLock(std::mutex & _mutex,std::condition_variable & _cv, bool & _served)
    : lock(_mutex), moved(false), served(_served)
    { 
        _cv.wait(lock, [&] { return _served; });
        _served = false;
    }

    template<class T>
    Serve<T>::ConsumeLock::ConsumeLock(ConsumeLock&& other)
    : lock(std::move(other.lock)), served(other.served)
    { 
        other.moved = true;
    }

    template<class T>
    Serve<T>::ConsumeLock::~ConsumeLock()
    {
        if(moved)
            return;
        
        served = false;
    }

    // Definition of ServeLock

    template<class T>
    Serve<T>::ServeLock::ServeLock(std::mutex & _mutex,std::condition_variable & _cv, bool & _served)
    : lock(_mutex),cv(_cv),moved(false), served(_served)
    { }

    template<class T>
    Serve<T>::ServeLock::ServeLock(ServeLock&& other)
    : served(other.served),cv(other.cv),lock(std::move(other.lock)),moved(false)
    {
        other.moved = true;
    }

    template<class T>
    Serve<T>::ServeLock::~ServeLock()
    {
        if(moved)
            return;

        served = true;
        lock.unlock();
        cv.notify_all();
    }

    // Definition of Serve

    template<class T>
    Serve<T>::Serve()
    : Serve(nullptr)
    { }

    template<class T>
    Serve<T>::Serve(std::unique_ptr<T> const& _object)
    : object(&_object),serveMutex(),serveConditionVariable(),served(false)
    { }

    template<class T>
    Serve<T>::~Serve()
    { }

    template<class T>
    void Serve<T>::SetObject(std::unique_ptr<T> const& _object)
    {
        object = &_object;
    }

    template<class T>
    bool Serve<T>::IsObjectSet()
    {
        return object != nullptr;
    }

    template<class T>
    std::unique_ptr<T> const& Serve<T>::GetPointer()
    {
        return *object;
    }

    template<class T>
    Serve<T>::ConsumeLock Serve<T>::GetConsumeLock()
    {
        return std::move(ConsumeLock(serveMutex,serveConditionVariable,served));
    }

    template<class T>
    void Serve<T>::Flush()
    {
        ServeLock lock;
    }

    template<class T>
    Serve<T>::ServeLock Serve<T>::GetServeLock()
    {
        return std::move(ServeLock(serveMutex,serveConditionVariable,served));
    }

    template<class T>
    bool Serve<T>::WasConsumed()
    {
        std::lock_guard lock(serveMutex);
        return WasConsumedNoLock();
    }

    template<class T>
    bool Serve<T>::WasConsumedNoLock()
    {
        return !served;
    }
}