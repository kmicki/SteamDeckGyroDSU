#include "hiddevreader.h"
#include "cemuhookadapter.h"
#include "log.h"

#include <iostream>
#include <sstream>
#include <fstream>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    // Constants
    const int HidDevReader::cInputRecordLen = 8;   // Number of bytes that are read from hiddev file per 1 byte of HID data.

    HidDevReader::HidDevReader(int hidNo, int _frameLen, int scanTime) 
    :   frameLen(_frameLen),
        frame(_frameLen),
        scanPeriod(scanTime),
        mainMutex(),frameMutex(),clientDeliveredMutex(),startStopMutex(),
        readTaskMutex(),stopMetroMutex(),tickMutex(),
        readTaskProceed(),frameProceed(),newFrameProceed(),nextFrameProceed(),
        frameLockClients(),frameDeliveredClients()
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
        std::lock_guard startLock(startStopMutex); // prevent starting and stopping at the same time

        Log("HidDevReader: Attempting to start frame grab...");
        if(readingTask != nullptr)
            return; // already started

        stopTask = false;

        inputStream.reset(new std::ifstream(inputFilePath,std::ios_base::binary));

        if(inputStream->fail())
            throw std::runtime_error("Failed to open hiddev file. Are priviliges granted?");

        readingTask.reset(new std::thread(&HidDevReader::mainTask,std::ref(*this)));
        Log("HidDevReader: Started frame grab.");
    }
    
    void HidDevReader::Stop()
    {
        std::lock_guard startLock(startStopMutex); // prevent starting and stopping at the same time

        Log("HidDevReader: Attempting to stop frame grab...");

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

        Log("HidDevReader: Stopped frame grab.");
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
        Log("HidDevReader: Reopening hiddev file...");
        stream.close();
        stream.clear();
        stream.open(path);
    }

    // Extract HID frame from data read from /dev/usb/hiddevX
    void HidDevReader::processData(std::vector<char> const& bufIn) 
    {
        // Constants
        static const int cByteposInput = 4;     // Position in the raw hiddev record (of INPUT_RECORD_LEN length) where 
                                                // HID data byte is.

        // Each byte is encapsulated in a record
        for (int i = 0, j = cByteposInput; i < frame.size(); ++i,j+=cInputRecordLen) {
            frame[i] = bufIn[j];
        }
    }

    void HidDevReader::readTask(std::unique_ptr<std::vector<char>> const& buf)
    {
        Log("HidDevReader: HID data reading task started.");
        std::unique_lock<std::mutex> lock(readTaskMutex);
        while(!stopRead)
        {
            readTaskProceed.wait(lock,[&] { return !hidDataReady; });
            lock.unlock();

            {
                std::unique_lock<std::mutex> tickLock(tickMutex);
                readTaskProceed.wait(tickLock,[&] { return readTick; });
                readTick=false;
            }

            inputStream->read(buf->data(),buf->size());

            lock.lock();
            hidDataReady = true;
            lock.unlock();
            readTaskProceed.notify_all();
            lock.lock();
        }
        Log("HidDevReader: HID data reading task stopped normally.");
    }

    void HidDevReader::Metronome()
    {
        Log("HidDevReader: Metronome started.");
        std::unique_lock<std::mutex> stopLock(stopMetroMutex);
        while(!stopMetro)
        {
            stopLock.unlock();
            std::this_thread::sleep_for(scanPeriod);
            {
                std::lock_guard<std::mutex> lock(tickMutex);
                readTick = true;
            }
            readTaskProceed.notify_all();
            stopLock.lock();
        }
        Log("HidDevReader: Metronome stopped.");
    }

    bool HidDevReader::LossAnalysis(int64_t const& diff)
    {
        // Constants
        static const int cMaxLossPeriod = 10000;
        static const std::chrono::seconds lossAnalysisMinWindow(1);     // Minimum time between frame losses to execute loss analysis

        if(lastInc == 0)
            return false;

        if(readStuck > 0)
        {
            --readStuck;
            if (diff > 1)
            {
                readStuck = 0;
                scanPeriod += std::chrono::microseconds(std::min(200,std::max(1,(6000-lossPeriod)/800)));
                { LogF msg; msg << "Changed scan period to : " << scanPeriod.count() << " us"; }
                lossPeriod = 0;
                lossAnalysis = false;
                return diff > 1 && lastInc > 0 && diff <= 40;
            }
        }
        
        if(passedNoLossAnalysis || std::chrono::system_clock::now()-lastAnalyzedFrameLossTime > lossAnalysisMinWindow)
        {
            passedNoLossAnalysis = true;

            bool result = false;
            if(lossAnalysis)
            {
                if(++lossPeriod > cMaxLossPeriod)
                {
                    { LogF msg; msg << "HidDevReader: Over " << cMaxLossPeriod << " frames since last frame loss."; }
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
                    if(lossAnalysis)
                    {
                        scanPeriod -= std::chrono::microseconds(std::min(200,std::max(1,(6000-lossPeriod)/400)));
                        { LogF msg; msg << "HidDevReader: Changed scan period to : " << scanPeriod.count() << " us"; }
                    }
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


            if(result)
            {
                lastAnalyzedFrameLossTime = std::chrono::system_clock::now();
                passedNoLossAnalysis = false;
            }
        }

        return diff > 1 && lastInc > 0 && diff <= 40;
    }

    void HidDevReader::IntializeLossAnalysis()
    {
        lossPeriod = 0;         // number of packets since last loss of frames
        lossAnalysis = false;   // loss analysis is active
        lastInc = 0;            
        smallLossEncountered = false;
        lastAnalyzedFrameLossTime = std::chrono::system_clock::now();
        passedNoLossAnalysis = false; // Flag that minimum window between lost frames has passed in order to execute loss analysis
        readStuck = 0;
    }

    template<class Mutex,class CondVar,class Rep, class Period, class Predicate>
    bool WaitForSignal(Mutex &mutex, CondVar &condVar, Predicate stop, std::chrono::duration<Rep,Period> const& timeout)
    {
        std::unique_lock lock(mutex);
        return condVar.wait_for(lock,timeout,stop);
    }

    void HidDevReader::mainTask()
    {
        // Constants
        static const std::chrono::microseconds readTaskKillTime(1);   // Time between signaling read task to stop and attempting to force kill the thread.
        // readTask sometimes hangs on reading from hiddev file forever. This has to be detected and thread needs to be killed forcefully.
        static const std::chrono::milliseconds readTaskTimeout(6);      // Timeout of readTask - means that reading from hiddev file hangs

        IntializeLossAnalysis();

        // HID Data Buffers
        auto bufferLength = cInputRecordLen*frameLen;
        std::unique_ptr<std::vector<char>> hidDataBuffer(new std::vector<char>(bufferLength)); // main buffer
        std::unique_ptr<std::vector<char>> hidDataBufferProcess(new std::vector<char>(bufferLength)); // secondary buffer for processing

        stopRead=readTick=hidDataReady=false;   // reset readTask control flow flags
        stopMetro = false;

        std::unique_ptr<std::thread> readThread(new std::thread(&HidDevReader::readTask,this,std::cref(hidDataBuffer)));  
        auto handle = readThread->native_handle();

        std::thread metronome(&HidDevReader::Metronome,this);

        { LogF msg; msg << "HidDevReader: Scan period initial : " << scanPeriod.count() << " us"; }

        std::unique_lock mainLock(mainMutex);
        while(!stopTask)
        {
            mainLock.unlock();

            

            if(!WaitForSignal(readTaskMutex,readTaskProceed,[&] { return hidDataReady; },readTaskTimeout))
            {
                Log("HidDevReader: Reading from hiddev file stuck. Force-restarting reading task.");
                readStuck = 10;
                {
                    std::lock_guard lock(readTaskMutex);
                    stopRead = true;
                    readTick = true;
                    hidDataReady = false;
                }
                readTaskProceed.notify_all();

                std::this_thread::sleep_for(readTaskKillTime);
                pthread_cancel(handle);
                readThread->join();
                {
                    std::lock_guard lock(readTaskMutex);
                    stopRead = false;
                    if(!hidDataReady)
                        readTick = true;
                }
                reconnectInput(*inputStream,inputFilePath);
                readThread.reset(new std::thread(&HidDevReader::readTask,this,std::cref(hidDataBuffer)));
                handle = readThread->native_handle();

                mainLock.lock();
                continue;
            }

            bool inputFail;
            if((inputFail = inputStream->fail()) || *(reinterpret_cast<unsigned int*>(hidDataBuffer->data())) != 0xFFFF0002)
            {
                if(inputFail)
                    Log("HidDevReader: Reading from hiddev file failed.");
                else
                    Log("HidDevReader: Reading from hiddev file started in the middle of the HID frame.");
                // Failed to read a frame
                // or start in the middle of the input frame
                {
                    std::lock_guard lock(readTaskMutex);
                    reconnectInput(*inputStream,inputFilePath);
                    readStuck = 10;
                    hidDataReady = false; readTick = true;
                }
                readTaskProceed.notify_all();

                mainLock.lock();
                continue;
            }

            hidDataBuffer.swap(hidDataBufferProcess);
            {
                std::lock_guard<std::mutex> lock(readTaskMutex);
                hidDataReady = false;
            }
            readTaskProceed.notify_all();
            
            {
                std::unique_lock frameLock(frameMutex);
                nextFrameProceed.wait(frameLock,[&] { return frameLockClients.empty(); } );
                processData(*hidDataBufferProcess);
            }

            {
                std::lock_guard lock(clientDeliveredMutex);
                frameDeliveredClients.clear();
            }
            newFrameProceed.notify_all();

            uint32_t & newInc = *reinterpret_cast<uint32_t *>(frame.data()+4);
            int64_t diff = newInc - lastInc;

            LossAnalysis(diff);

            lastInc = newInc;

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
        std::this_thread::sleep_for(readTaskKillTime);
        pthread_cancel(handle);
        readThread->join();
        metronome.join();
        inputStream->close();
    }
}