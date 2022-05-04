#include "hiddevreader.h"

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
    #define INPUT_FRAME_TIMEOUT_MS 5

    HidDevReader::HidDevReader(int hidNo, int _frameLen, int scanTime) 
    :   frameLen(_frameLen),
        frame(_frameLen),
        preReadingLock(false),
        readingLock(false),
        writingLock(false),
        stopTask(false),
        scanPeriod(scanTime),
        inputStream(nullptr),
        frameReadAlready(false),
        clients()
    {
        if(hidNo < 0) throw std::invalid_argument("hidNo");

        std::stringstream inputFilePathFormatter;
        inputFilePathFormatter << "/dev/usb/hiddev" << hidNo;
        inputFilePath = inputFilePathFormatter.str();

        inputStream.reset(new std::ifstream(inputFilePath,std::ios_base::binary));

        if(inputStream.get()->fail())
            throw std::runtime_error("Failed to open hiddev file. Are priviliges granted?");

        readingTask.reset(new std::thread(&HidDevReader::executeReadingTask,std::ref(*this)));
    }

    HidDevReader::~HidDevReader()
    {
        stopTask = true;
        if(readingTask.get() != nullptr)
            readingTask.get()->join();
    }

    HidDevReader::frame_t const& HidDevReader::GetFrame(void * client)
    {
        if(clients.find(client) == clients.end())
            clients.insert(client);
        LockFrame(client);
        return frame;
    }

    HidDevReader::frame_t const& HidDevReader::GetNewFrame(void * client)
    {
        while(clients.find(client) != clients.end()) ;
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
        if(clientLocks.find(client) == clientLocks.end())
            clientLocks.insert(client);
        if(! preReadingLock && ! readingLock)
        {
            preReadingLock = true;
            while(writingLock) ;
            readingLock = true;
            preReadingLock = false;
        }
    }

    void HidDevReader::UnlockFrame(void * client)
    {
        auto iter = clientLocks.find(client);
        if(iter != clientLocks.end())
            clientLocks.erase(iter);
        if(clientLocks.size() == 0)
            readingLock = false;
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
        writingLock = false;
        frameReadAlready = false;

        clients.clear();
    }

    void HidDevReader::executeReadingTask()
    {
        auto& input = static_cast<std::ifstream&>(*inputStream);
        std::unique_ptr<std::future<void>> scan = nullptr;
        std::unique_ptr<std::future<void>> process = nullptr;
        std::vector<char> buf1(INPUT_RECORD_LEN*frameLen);
        std::vector<char> buf2(INPUT_RECORD_LEN*frameLen);
        std::vector<char>* buf = &buf1; // buffer swapper

        while(!stopTask)
        {
            if(scan.get() != nullptr)
                scan.get()->wait();
            scan.reset(new std::future<void>(std::async(std::launch::async,std::this_thread::sleep_for<int64_t,std::milli>,scanPeriod)));

            // Reading from hiddev is done in a separate thread.
            // Then future is created that waits for the reading to finish (by executing join).
            // It is necessary because if 2 consecutive reads are done too quickly (no new frame ready)
            // then ifstream::read is blocked indefinitely.
            std::thread readThread(&std::ifstream::read,&input,buf->data(),buf->size());
            auto handle = readThread.native_handle();   // this handle is needed to cancel the reading thread in case of block,
                                                        // it has to be retrieved and stored before calling join on the thread.
            auto readTimeout=std::async(std::launch::async, &std::thread::join, &readThread);
            if (readTimeout.wait_for(std::chrono::milliseconds(INPUT_FRAME_TIMEOUT_MS)) == std::future_status::timeout) {
                // If reading didn't finish in defined time, cancel reading thread and repeat reading in next iteration
                pthread_cancel(handle);
                readTimeout.wait();
                reconnectInput(input,inputFilePath);
            }
            else if(input.fail() || *(reinterpret_cast<unsigned int*>(buf->data())) != 0xFFFF0002)
            {
                // Failed to read a frame
                // or start in the middle of the input frame
                reconnectInput(input,inputFilePath);
            }
            else {
                // Start processing data in a separate thread unless previous processing has not finished
                if( !readingLock && !preReadingLock 
                    && 
                    (process.get() == nullptr || process.get()->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready))
                {
                    writingLock = true;
                    process.reset(new std::future<void>(std::async(std::launch::async, &HidDevReader::processData, this, std::cref(*buf))));

                    // swap buffers
                    if(buf == &buf1)
                        buf = &buf2;
                    else
                        buf = &buf1;
                }
            }
        }
        if(process.get() != nullptr)
            process.get()->wait();
        input.close();
    }
}