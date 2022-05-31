#ifndef _KMICKI_HIDDEV_HIDDEVREADER_H_
#define _KMICKI_HIDDEV_HIDDEVREADER_H_

#include <vector>
#include <string>
#include <thread>
#include <fstream>
#include <set>
#include <mutex>
#include <condition_variable>

namespace kmicki::hiddev 
{
    // Reads periodic data from a given HID device (/dev/usb/hiddevX)
    // in constant-length frames and provides most recent frame.
    class HidDevReader
    {
        public:

        typedef std::vector<char> frame_t;

        HidDevReader() = delete;

        // Constructor.
        // Starts reading task.
        // hidNo: ID of HID device (X in /dev/usb/hiddevX)
        // frameLen: Size of single HID data frame
        // scanTime: Period between frames in ms. 
        //           If it will be around or lower than actual period of incoming frames,
        //           The reading task will block often and will have to reinitialize reading.
        //           If it will be much higher then the generated frames will be out of sync
        //           (a block of consecutive frames and then skip)
        HidDevReader(int hidNo, int _frameLen, int scanTime = 0);

        // Destructor. 
        // Stops reading task.
        // Closes input file.
        ~HidDevReader();

        // Set start marker in case hiddev file doesn't provide a way to find start of the frame.
        void SetStartMarker(std::vector<char> const& marker);

        // Get current frame data.
        // Locks frame for reading.
        // If frame is locked for writing, it waits until writing is finished.
        // After finishing reading the frame, it is improtant to call UnlockFrame().
        frame_t const& GetFrame();

        frame_t const& Frame();

        // Get current frame data.
        // Locks frame for reading.
        // If frame is locked for writing, it waits until writing is finished.
        // Blocks if there was no new frame after last use of GetFrame or GetNewFrame
        frame_t const& GetNewFrame();

        // Unlock frame.
        void UnlockFrame();

        void Start();

        void Stop();

        bool IsStarted();

        bool IsStopping();

        std::mutex frameMutex;

        protected:

        bool frameDelivered;

        frame_t frame;

        int frameLen;
        std::string inputFilePath;

        std::chrono::microseconds scanPeriod;

        bool stopTask; // stop reading task
        bool stopRead;
        bool stopMetro;

        std::unique_ptr<std::thread> readingTask;

        // Method called in a reading task on a separate thread.
        void executeReadingTask();
        void readTask(std::vector<char>** buf);
        void Metronome();

        std::mutex m;
        std::condition_variable v;

        int enter,exit;
        bool tick,rdy,wait;

        static void reconnectInput(std::ifstream & stream, std::string path);
        void processData(std::vector<char> const& bufIn);        

        std::unique_ptr<std::ifstream> inputStream;

        std::mutex startStopMutex;

        bool LossAnalysis(int64_t const& diff);

        int lossPeriod;
        bool lossAnalysis;
        uint32_t lastInc;
        bool smallLossEncountered;
        std::chrono::microseconds bigLossDuration;

        std::vector<char> startMarker;
    };

}

#endif