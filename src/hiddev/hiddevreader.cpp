#include "hiddevreader.h"
#include "cemuhookadapter.h"
#include "log.h"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <chrono>
#include <future>
#include <memory>
#include <set>

using namespace kmicki::log;

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
        v(),m()
    {
        if(hidNo < 0) throw std::invalid_argument("hidNo");

        std::stringstream inputFilePathFormatter;
        inputFilePathFormatter << "/dev/usb/hiddev" << hidNo;
        inputFilePath = inputFilePathFormatter.str();
        bigLossDuration = std::chrono::microseconds(4000);

        Log("HidDevReader: Initialized. Waiting for start of frame grab.");
    }

    void HidDevReader::Start()
    {
        Log("HidDevReader: Attempting to start frame grab...");
        startStopMutex.lock();
        if(readingTask.get() != nullptr)
            return;
        stopTask = false;
        inputStream.reset(new std::ifstream(inputFilePath,std::ios_base::binary));

        if(inputStream.get()->fail())
            throw std::runtime_error("HidDevReader: Failed to open hiddev file. Are priviliges granted?");

        readingTask.reset(new std::thread(&HidDevReader::executeReadingTask,std::ref(*this)));
        startStopMutex.unlock();
        Log("HidDevReader: Started frame grab.");
    }
    
    void HidDevReader::Stop()
    {
        Log("HidDevReader: Attempting to stop frame grab...");
        startStopMutex.lock();
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
        Log("HidDevReader: Reopening hiddev file...");
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
        Log("HidDevReader: HID data reading task started.");
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
        Log("HidDevReader: HID data reading task stopped normally.");
    }

    void HidDevReader::Metronome()
    {
        Log("HidDevReader: Metronome started.");
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
        Log("HidDevReader: Metronome stopped.");
    }

    bool HidDevReader::LossAnalysis(int64_t const& diff)
    {
        bool result = false;
        if(lossAnalysis)
        {
            if(++lossPeriod > MAX_LOSS_PERIOD)
            {
                { LogF msg; msg << "HidDevReader: Over " << MAX_LOSS_PERIOD << " frames since last frame loss."; }
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
                { LogF msg; msg << "HidDevReader: Changed scan period to : " << scanPeriod.count() << " us"; }
                lossAnalysis = true;
                result = true;
            }
            else if(!lossAnalysis)
                lossAnalysis = true;
            else
            {
                smallLossEncountered = true;
                ++scanPeriod;
                { LogF msg; msg << "HidDevReader: Changed scan period to : " << scanPeriod.count() << " us"; }
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

        { LogF msg; msg << "HidDevReader: Scan period initial : " << scanPeriod.count() << " us"; }

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
                        Log("HidDevReader: Reading from hiddev file stuck. Force-restarting reading task.");
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
            bool inputFail;

            if((inputFail = input.fail()) || *(reinterpret_cast<unsigned int*>(buf->data())) != 0xFFFF0002)
            {
                if(inputFail)
                    Log("HidDevReader: Reading from hiddev file failed.");
                else
                    Log("HidDevReader: Reading from hiddev file started in the middle of the HID frame.");
                // Failed to read a frame
                // or start in the middle of the input frame
                {
                    std::unique_lock<std::mutex> lock(m);
                    reconnectInput(input,inputFilePath);
                    rdy = false; tick = true;
                    if(wait)
                        v.notify_one();
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
                    std::unique_lock<std::mutex> lock(m);
                    rdy = false;
                    if(tick && wait)
                        v.notify_one();
                }
                
                processData(*bufProcess);

                uint32_t * newInc = reinterpret_cast<uint32_t *>(frame.data()+4);
                int64_t diff = *newInc - lastInc;

                if(lastInc != 0)
                {
                    if(diff > 1)
                        { LogF msg; msg << "HidDevReader: Missed " << (diff-1) << " frames."; }
                    else if(diff == 0)
                        Log("HidDevReader: Last frame repeated.");
                    else if(diff < 0)
                        { LogF msg; msg << "HidDevReader: Frame " << diff << " repeated."; }
                }

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
                    Log("HidDevReader: Replicating missing frames.");
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
                if(diff > 0 || diff < 10000)
                    lastInc = *newInc;

                frameDelivered = false;
                frameMutex.unlock();
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