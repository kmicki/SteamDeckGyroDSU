#include "hiddevreader.h"
#include "cemuhookadapter.h"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <chrono>
#include <future>
#include <memory>
#include <set>

namespace kmicki::hiddev
{
    #define INPUT_RECORD_LEN 8
    #define BYTEPOS_INPUT 4
    #define FAIL_COUNT 10
    #define MAX_LOSS_PERIOD 10000


    HidDevReader::HidDevReader(int hidNo, int _frameLen, int scanTime) 
    :   frameLen(_frameLen),
        frame(_frameLen),
        preReadingLock(false),readingLock(false),writingLock(false),
        stopTask(false),stopMetro(false),stopRead(false),
        scanPeriod(scanTime),
        inputStream(nullptr),
        clients(),clientLocks(),
        lockMutex(),clientsMutex(),startStopMutex(),
        v(),m()
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
        startStopMutex.lock();
        std::cout << "HidDevReader: Attempting to start frame grab." << std::endl;
        if(readingTask.get() != nullptr)
            return;
        stopTask = false;
        inputStream.reset(new std::ifstream(inputFilePath,std::ios_base::binary));

        if(inputStream.get()->fail())
            throw std::runtime_error("Failed to open hiddev file. Are priviliges granted?");

        readingTask.reset(new std::thread(&HidDevReader::executeReadingTask,std::ref(*this)));
        startStopMutex.unlock();
        std::cout << "HidDevReader: Started frame grab." << std::endl;
    }
    
    void HidDevReader::Stop()
    {
        startStopMutex.lock();
        std::cout << "HidDevReader: Attempting to stop frame grab." << std::endl;
        stopTask = true;
        if(readingTask.get() != nullptr)
        {
            readingTask.get()->join();
            readingTask.reset();
        }
        startStopMutex.unlock();
        std::cout << "HidDevReader: Stopped frame grab." << std::endl;
    }

    bool HidDevReader::IsStarted()
    {
        return readingTask.get() != nullptr && !stopTask;
    }

    bool HidDevReader::IsStopping()
    {
        return readingTask.get() != nullptr && stopTask;
    }

    HidDevReader::~HidDevReader()
    {
        Stop();
    }

    HidDevReader::frame_t const& HidDevReader::GetFrame(void * client)
    {
        LockFrame(client);
        clientsMutex.lock();
        if(clients.find(client) == clients.end())
            clients.insert(client);
        clientsMutex.unlock();
        return frame;
    }

    HidDevReader::frame_t const& HidDevReader::Frame()
    {
        return frame;
    }

    HidDevReader::frame_t const& HidDevReader::GetNewFrame(void * client)
    {
        while(clients.find(client) != clients.end() && !stopTask) std::this_thread::sleep_for(std::chrono::microseconds(100));
        return GetFrame(client);
    }

    bool const& HidDevReader::IsFrameLockedForWriting() const
    {
        return writingLock;
    }
    bool const& HidDevReader::IsFrameLockedForReading() const
    {
        return readingLock;
    }

    void HidDevReader::LockFrame(void* client)
    {
        lockMutex.lock();
        if(clientLocks.find(client) == clientLocks.end())
            clientLocks.insert(client);
        if(! preReadingLock && ! readingLock)
        {
            preReadingLock = true;
            while(writingLock) ;
            readingLock = true;
            preReadingLock = false;
        }
        lockMutex.unlock();
    }

    void HidDevReader::UnlockFrame(void * client)
    {
        lockMutex.lock();
        std::set<void*>::iterator iter;
        while((iter = clientLocks.find(client)) != clientLocks.end())
            clientLocks.erase(iter);
        if(clientLocks.size() == 0)
            readingLock = false;
        lockMutex.unlock();
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
        while(!stopRead)
        {
            tick=false;
            ++enter;
            inputStream->read((*buf)->data(),(*buf)->size());
            if(stopRead)
                break;
            exit=enter;
            rdy = true;
            if(!tick || rdy)
            {
                std::unique_lock<std::mutex> lock(m);
                wait=true;
                v.wait(lock);
                wait=false;
            }
        }
    }

    void HidDevReader::Metronome()
    {
        while(!stopMetro)
        {
            std::this_thread::sleep_for(scanPeriod);
            {
                std::unique_lock<std::mutex> lock(m);
                tick = true;
                if(!rdy && wait)
                    v.notify_one();
            }
        }
    }

    void HidDevReader::LossAnalysis(uint32_t diff)
    {
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
            }
            else if(!lossAnalysis)
                lossAnalysis = true;
            else
            {
                smallLossEncountered = true;
                ++scanPeriod;
                std::cout << "Changed scan period to : " << scanPeriod.count() << " us" << std::endl;
            }
            lossPeriod = 0;
        }
    }

    void HidDevReader::executeReadingTask()
    {
        auto& input = static_cast<std::ifstream&>(*inputStream);
        std::vector<char> buf1_1(INPUT_RECORD_LEN*frameLen);
        std::vector<char> buf1_2(INPUT_RECORD_LEN*frameLen);
        std::vector<char>* buf = &buf1_1; // buffer swapper

        enter=exit=0;
        tick=rdy=wait=false;
        int fail = 0;
        int lastEnter = 0;

        stopRead = stopMetro = false;

        std::unique_ptr<std::thread> readThread(new std::thread(&HidDevReader::readTask,this,&buf));
        auto handle = readThread->native_handle();
        std::thread metronome(&HidDevReader::Metronome,this);

        lossPeriod = 0;
        lossAnalysis = false;
        lastInc = 0;
        smallLossEncountered = false;

        std::cout << "Scan period initial : " << scanPeriod.count() << " us" << std::endl;

        while(!stopTask)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(500));

            if(!rdy)
            {
                if(enter!=exit)
                {
                    if(enter != lastEnter) 
                    {
                        fail = 0;
                        lastEnter = enter;
                    }
                    ++fail;
                    if(fail > FAIL_COUNT)
                    {
                        stopRead = true;
                        {
                            std::unique_lock<std::mutex> lock(m);
                            if(wait)
                                v.notify_one();
                        }
                        std::this_thread::sleep_for(std::chrono::microseconds(300));
                        pthread_cancel(handle);
                        readThread->join();
                        stopRead = false;
                        tick=true;
                        readThread.reset(new std::thread(&HidDevReader::readTask,this,&buf));
                        handle = readThread->native_handle();
                        fail=0;
                    }
                }
                continue;
            }
            fail=0;
            
            if(input.fail() || *(reinterpret_cast<unsigned int*>(buf->data())) != 0xFFFF0002)
            {
                // Failed to read a frame
                // or start in the middle of the input frame
                reconnectInput(input,inputFilePath);
            }
            else {
                if( readingLock || preReadingLock )
                    std::this_thread::sleep_for(std::chrono::microseconds(200));
                // Start processing data in a separate thread unless previous processing has not finished
                if( !readingLock && !preReadingLock )
                {
                    writingLock = true;
                    auto bufProcess = buf;
                    // swap buffers
                    if(buf == &buf1_1)
                        buf = &buf1_2;
                    else
                        buf = &buf1_1;
                    {
                        std::unique_lock<std::mutex> lock(m);
                        rdy = false;
                        if(tick && wait)
                            v.notify_one();
                    }
                    
                    processData(*buf);

                    uint32_t * newInc = reinterpret_cast<uint32_t *>(frame.data()+4);
                    uint32_t diff = *newInc - lastInc;
                    LossAnalysis(diff);

                    if(diff > 1 && lastInc > 0 && diff <= 40)
                    {
                        *newInc = lastInc+1;
                        // Fill with generated frames to avoid jitter
                        auto fillPeriod = std::chrono::microseconds(3200/(diff-1));//3000/(diff-1));

                        for (int i = 0; i < diff-1; i++)
                        {
                            writingLock=false;
                            clientsMutex.lock();
                            clients.clear();
                            clientsMutex.unlock();
                            std::this_thread::sleep_for(fillPeriod);
                            if( !readingLock && !preReadingLock )
                                writingLock = true;
                            ++(*newInc);
                        }
                    }
                    lastInc = *newInc;

                    writingLock = false;
                    
                    clientsMutex.lock();
                    clients.clear();
                    clientsMutex.unlock();
                    
                }
            }
        }
        stopRead = stopMetro = true;
        {
            std::unique_lock<std::mutex> lock(m);
            if(wait)
                v.notify_one();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        pthread_cancel(handle);
        readThread->join();
        metronome.join();
        input.close();
    }
}