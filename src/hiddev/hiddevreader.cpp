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
        frame(_frameLen),frameDelivered(false),
        stopTask(false),stopMetro(false),stopRead(false),
        scanPeriod(scanTime),
        inputStream(nullptr),
        frameMutex(),startStopMutex(),
        readTaskProceed(),readTaskMutex()
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

    HidDevReader::frame_t const& HidDevReader::GetFrame()
    {
        frameMutex.lock();
        frameDelivered = true;
        return frame;
    }

    HidDevReader::frame_t const& HidDevReader::Frame()
    {
        return frame;
    }

    HidDevReader::frame_t const& HidDevReader::GetNewFrame()
    {
        while(frameDelivered && !stopTask) std::this_thread::sleep_for(std::chrono::microseconds(10));
        return GetFrame();
    }

    void HidDevReader::UnlockFrame()
    {
        frameMutex.unlock();
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
        std::unique_lock<std::mutex> lockStop(readTaskMutex);
        lockStop.lock();
        while(!stopRead)
        {
            lockStop.unlock();
            tick=false;
            ++readTaskEnter;
            inputStream->read((*buf)->data(),(*buf)->size());
            {
                std::lock_guard<std::mutex> lock(readTaskMutex);
                if(stopRead)
                    break;
            }
            readTaskExit=readTaskEnter;
            rdy = true;
            {
                std::unique_lock<std::mutex> lock(readTaskMutex);
                readTaskProceed.wait(lock,[&] { return tick && !rdy; });
            }
            lockStop.lock();
        }
    }

    void HidDevReader::Metronome()
    {
        while(!stopMetro)
        {
            std::this_thread::sleep_for(scanPeriod);
            {
                std::lock_guard<std::mutex> lock(readTaskMutex);
                tick = true;
            }
            readTaskProceed.notify_one();
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
        std::vector<char> buf1_1(INPUT_RECORD_LEN*frameLen);
        std::vector<char> buf1_2(INPUT_RECORD_LEN*frameLen);
        std::vector<char>* buf = &buf1_1; // buffer swapper

        auto startTime = std::chrono::system_clock::now();
        bool passedNoLossAnalysis = false;

        readTaskEnter=readTaskExit=0;
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
                if(readTaskEnter!=readTaskExit)
                {
                    if(readTaskEnter != lastEnter) 
                    {
                        fail = 0;
                        lastEnter = readTaskEnter;
                    }
                    ++fail;
                    if(fail > FAIL_COUNT)
                    {
                        {
                            std::lock_guard<std::mutex> lock(readTaskMutex);
                            stopRead = true;
                            tick = true;
                            rdy = false;
                        }
                        readTaskProceed.notify_one();

                        std::this_thread::sleep_for(std::chrono::microseconds(300));
                        pthread_cancel(handle);
                        readThread->join();
                        stopRead = false;
                        tick = true;
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
                {
                    std::lock_guard<std::mutex> lock(readTaskMutex);
                    reconnectInput(input,inputFilePath);
                    rdy = false; tick = true;
                    if(wait)
                        readTaskProceed.notify_one();
                }
            }
            else {
                frameMutex.lock();
                // Start processing data in a separate thread unless previous processing has not finished

                auto bufProcess = buf;
                // swap buffers
                if(buf == &buf1_1)
                    buf = &buf1_2;
                else
                    buf = &buf1_1;
                {
                    std::lock_guard<std::mutex> lock(readTaskMutex);
                    rdy = false;
                    if(tick && wait)
                        readTaskProceed.notify_one();
                }
                
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

                    // Flag - replicated frames
                    frame[0] = 0xDD;

                    for (int i = 0; i < diff-1; i++)
                    {
                        frameDelivered = false;
                        frameMutex.unlock();
                        std::this_thread::sleep_for(fillPeriod);
                        frameMutex.lock();
                        ++(*newInc);
                    }

                    frame[0] = 0x01;
                }
                lastInc = *newInc;

                frameDelivered = false;
                frameMutex.unlock();
            }
        }
        stopRead = stopMetro = true;
        {
            std::lock_guard<std::mutex> lock(readTaskMutex);
            if(wait)
                readTaskProceed.notify_one();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        pthread_cancel(handle);
        readThread->join();
        metronome.join();
        input.close();
    }
}