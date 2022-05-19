#include "hiddevreader.h"
#include "cemuhookadapter.h"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <chrono>
#include <future>
#include <memory>
#include <set>
#include <algorithm>

namespace kmicki::hiddev
{
    #define INPUT_RECORD_LEN 8
    #define BYTEPOS_INPUT 4
    #define FAIL_COUNT 10
    #define MAX_LOSS_PERIOD 10000

    HidDevReader::HidDevReader(int hidNo, int _frameLen, int scanTime) 
    :   frameLen(_frameLen),
        frame(_frameLen),
        stopTask(false),stopMetro(false),stopRead(false),
        scanPeriod(scanTime),
        inputStream(nullptr),
        mainMutex(),frameMutex(),clientDeliveredMutex(),startStopMutex(),
        readTaskMutex(),stopMetroMutex(),
        readTaskProceed(),frameProceed(),newFrameProceed(),nextFrameProceed(),
        frameLockClients(),frameDeliveredClients()
    {
        if(hidNo < 0) throw std::invalid_argument("hidNo");

        std::stringstream inputFilePathFormatter;
        inputFilePathFormatter << "/dev/usb/hiddev" << hidNo;
        inputFilePath = inputFilePathFormatter.str();
        bigLossDuration = std::chrono::microseconds(4000);

        std::cout << "HidDevReader: Initialized. Waiting for start of frame grab." << std::endl;
    }

    void HidDevReader::Start()
    {
        std::lock_guard startLock(startStopMutex); // prevent starting and stopping at the same time

        std::cout << "HidDevReader: Attempting to start frame grab." << std::endl;
        if(readingTask != nullptr)
            return; // already started

        {
            std::lock_guard lock(mainMutex);
            stopTask = false;
        }

        inputStream.reset(new std::ifstream(inputFilePath,std::ios_base::binary));

        if(inputStream->fail())
            throw std::runtime_error("Failed to open hiddev file. Are priviliges granted?");

        readingTask.reset(new std::thread(&HidDevReader::executeReadingTask,std::ref(*this)));
        std::cout << "HidDevReader: Started frame grab." << std::endl;
    }
    
    void HidDevReader::Stop()
    {
        std::lock_guard startLock(startStopMutex); // prevent starting and stopping at the same time

        std::cout << "HidDevReader: Attempting to stop frame grab." << std::endl;

        {
            std::lock_guard lock(mainMutex);
            stopTask = true;
        }

        if(readingTask != nullptr)
        {
            {
                std::lock_guard lock(frameMutex);
                frameLockClients.clear();
            }
            readingTask->join();
            readingTask.reset();
            {
                std::lock_guard lock(clientDeliveredMutex);
                frameDeliveredClients.clear();
            }
        }

        std::cout << "HidDevReader: Stopped frame grab." << std::endl;
    }

    bool HidDevReader::IsStarted()
    {
        std::scoped_lock lock(startStopMutex,mainMutex);
        return readingTask.get() != nullptr && !stopTask;
    }

    bool HidDevReader::IsStopping()
    {
        std::scoped_lock lock(startStopMutex,mainMutex);
        return readingTask.get() != nullptr && stopTask;
    }

    HidDevReader::~HidDevReader()
    {
        Stop();
    }

    HidDevReader::frame_t const& HidDevReader::GetFrame(void* client)
    {
        bool markClient = false;
        {
            std::lock_guard mainLock(mainMutex);
            markClient = !stopTask;
        }

        std::shared_lock lock(frameMutex);
        if(markClient)
        {
            frameLockClients.push_back(client);
            {
                std::lock_guard lock(clientDeliveredMutex);
                frameDeliveredClients.push_back(client);
            }
        }
        return frame;
    }

    HidDevReader::frame_t const& HidDevReader::Frame()
    {
        return frame;
    }

    HidDevReader::frame_t const& HidDevReader::GetNewFrame(void* client)
    {
        {
            std::shared_lock lock(clientDeliveredMutex);
            newFrameProceed.wait(  
                lock,
                [&]
                {
                    // returns true if provided client is not in the vector
                    return std::find(frameDeliveredClients.begin(),
                                    frameDeliveredClients.end(),
                                    client) == frameDeliveredClients.end();
                });
        }
        return GetFrame(client);
    }

    void HidDevReader::UnlockFrame(void* client)
    {
        {
            std::lock_guard lock(frameMutex);
            std::vector<void*>::const_iterator item = frameLockClients.begin();
            while(item != frameLockClients.end())
            {
                if(*item == client)
                    item = frameLockClients.erase(item);
                else
                    ++item;
            }
        }    
        nextFrameProceed.notify_all();       
    }

    void HidDevReader::reconnectInput(std::ifstream & stream, std::string path)
    {
        stream.close();
        stream.clear();
        stream.open(path);
    }

    // Extract HID frame from data read from /dev/usb/hiddevX
    void HidDevReader::processData(std::vector<char> const& bufIn) 
    {
        // Each byte is encapsulated in an 8-byte long record
        for (int i = 0, j = BYTEPOS_INPUT; i < frame.size(); ++i,j+=INPUT_RECORD_LEN) {
            frame[i] = bufIn[j];
        }
    }

    void HidDevReader::readTask(std::vector<char>** buf)
    {
        std::unique_lock<std::mutex> lock(readTaskMutex);
        while(!stopRead)
        {
            readTaskProceed.wait(lock,[&] { return readTick && !hidDataReady; });
            readTick=false;

            lock.unlock();

            inputStream->read((*buf)->data(),(*buf)->size());

            lock.lock();

            if(stopRead)
                break;
            hidDataReady = true;
            lock.unlock();
            readTaskProceed.notify_all();
            lock.lock();
        }
    }

    void HidDevReader::Metronome()
    {
        std::unique_lock<std::mutex> stopLock(stopMetroMutex);
        while(!stopMetro)
        {
            stopLock.unlock();
            std::this_thread::sleep_for(scanPeriod);
            {
                std::lock_guard<std::mutex> lock(readTaskMutex);
                readTick = true;
            }
            readTaskProceed.notify_all();
            stopLock.lock();
        }
    }

    bool HidDevReader::LossAnalysis(uint32_t diff)
    {
        bool result = false;
        if(lossAnalysis)
        {
            if(++lossPeriod > MAX_LOSS_PERIOD)
            {
                lossPeriod = 0;
                lossAnalysis = false;
                if(bigLossDuration.count() < 4000)
                    ++bigLossDuration;
                    
            }
        }
        if(lastInc > 0 && diff > 1 && diff < 1000)
        {
            if(diff > 25)
            {
                if(smallLossEncountered || !lossAnalysis)
                    --scanPeriod;
                else
                    scanPeriod -= std::chrono::microseconds(std::max(1,(6000-lossPeriod)/100));
                std::cout << "Changed scan period to : " << scanPeriod.count() << " us" << std::endl;
                lossAnalysis = true;
                result = true;
            }
            else if(!lossAnalysis)
                lossAnalysis = true;
            else
            {
                smallLossEncountered = true;
                ++scanPeriod;
                std::cout << "Changed scan period to : " << scanPeriod.count() << " us" << std::endl;
                result = true;
            }
            lossPeriod = 0;
        }
        return result;
    }

    void HidDevReader::executeReadingTask()
    {
        auto& input = static_cast<std::ifstream&>(*inputStream);
        std::vector<char> buf1(INPUT_RECORD_LEN*frameLen);
        std::vector<char> buf2(INPUT_RECORD_LEN*frameLen);
        std::vector<char>* buf = &buf1; // buffer swapper

        auto startTime = std::chrono::system_clock::now();
        bool passedNoLossAnalysis = false;
        int lastEnter = 0;

        {
            std::lock_guard lock(readTaskMutex);
            stopRead=readTick=hidDataReady=false;
        }
        {
            std::lock_guard lock(stopMetroMutex);
            stopMetro = false;
        }

        std::unique_ptr<std::thread> readThread(new std::thread(&HidDevReader::readTask,this,&buf));  
        auto handle = readThread->native_handle();

        std::thread metronome(&HidDevReader::Metronome,this);

        lossPeriod = 0;
        lossAnalysis = false;
        lastInc = 0;
        smallLossEncountered = false;

        std::cout << "Scan period initial : " << scanPeriod.count() << " us" << std::endl;

        std::unique_lock mainLock(mainMutex);
        while(!stopTask)
        {
            mainLock.unlock();

            {
                std::unique_lock lock(readTaskMutex);
                readTaskProceed.wait_for(lock,std::chrono::milliseconds(5),[&] { return hidDataReady; });
            }

            if(!hidDataReady)
            {
                {
                    std::lock_guard lock(readTaskMutex);
                    stopRead = true;
                    readTick = true;
                    hidDataReady = false;
                }
                readTaskProceed.notify_all();

                std::this_thread::sleep_for(std::chrono::microseconds(300));
                pthread_cancel(handle);
                readThread->join();
                {
                    std::lock_guard lock(readTaskMutex);
                    stopRead = false;
                    if(!hidDataReady)
                        readTick = true;
                }
                readThread.reset(new std::thread(&HidDevReader::readTask,this,&buf));
                handle = readThread->native_handle();

                mainLock.lock();
                continue;
            }

            if(input.fail() || *(reinterpret_cast<unsigned int*>(buf->data())) != 0xFFFF0002)
            {
                // Failed to read a frame
                // or start in the middle of the input frame
                {
                    std::lock_guard lock(readTaskMutex);
                    reconnectInput(input,inputFilePath);
                    hidDataReady = false; readTick = true;
                }
                readTaskProceed.notify_all();

                mainLock.lock();
                continue;
            }

            auto bufProcess = buf;

            // swap buffers
            if(buf == &buf1)
                buf = &buf2;
            else
                buf = &buf1;

            {
                std::lock_guard<std::mutex> lock(readTaskMutex);
                hidDataReady = false;
            }
            readTaskProceed.notify_all();
            
            std::unique_lock frameLock(frameMutex);

            nextFrameProceed.wait(frameLock,[&] { return frameLockClients.empty(); } );

            processData(*bufProcess);

            uint32_t * newInc = reinterpret_cast<uint32_t *>(frame.data()+4);
            uint32_t diff = *newInc - lastInc;

            if(passedNoLossAnalysis || std::chrono::system_clock::now()-startTime > std::chrono::seconds(1))
            {
                passedNoLossAnalysis = true;
                if(LossAnalysis(diff))
                {
                    startTime = std::chrono::system_clock::now();
                    passedNoLossAnalysis = false;
                }
            }

            if(diff > 1 && lastInc > 0 && diff <= 40)
            {
                *newInc = lastInc+1;
                // Fill with generated frames to avoid jitter
                auto fillPeriod = std::chrono::microseconds(3200/(diff-1));//3000/(diff-1));

                for (int i = 0; i < diff-1; i++)
                {
                    {
                        std::lock_guard lock(clientDeliveredMutex);
                        frameDeliveredClients.clear();
                    }
                    newFrameProceed.notify_all();
                    frameLock.unlock();
                    std::this_thread::sleep_for(fillPeriod);
                    frameLock.lock();
                    nextFrameProceed.wait(frameLock,[&] { return frameLockClients.empty(); } );
                    ++(*newInc);
                }
            }

            lastInc = *newInc;

            {
                std::lock_guard lock(clientDeliveredMutex);
                frameDeliveredClients.clear();
            }
            newFrameProceed.notify_all();
            frameLock.unlock();
            mainLock.lock();
        }
        {
            std::lock_guard lock(readTaskMutex);
            stopRead = true;
        }
        {
            std::lock_guard lock(stopMetroMutex);
            stopMetro = true;
        }
        readTaskProceed.notify_all();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        pthread_cancel(handle);
        readThread->join();
        metronome.join();
        input.close();
    }
}