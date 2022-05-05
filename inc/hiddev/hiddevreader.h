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

        // Get current frame data.
        // Locks frame for reading.
        // If frame is locked for writing, it waits until writing is finished.
        // After finishing reading the frame, it is improtant to call UnlockFrame().
        frame_t const& GetFrame(void * client = 0);

        // Get current frame data.
        // Locks frame for reading.
        // If frame is locked for writing, it waits until writing is finished.
        // Blocks if there was no new frame after last use of GetFrame or GetNewFrame
        frame_t const& GetNewFrame(void * client = 0);

        // Check if frame is currently being written to.
        // True means that GetFrame() will block until writing ends.
        bool const& IsFrameLockedForWriting() const;

        // Check if frame is locked for reading.
        // True means that new frame will not be generated.
        // Use UnlockFrame() to unlock it.
        bool const& IsFrameLockedForReading() const;

        // Lock frame for reading.
        // If frame is locked for writing, it waits until writing is finished.
        void LockFrame(void * client = 0);

        // Unlock frame.
        void UnlockFrame(void * client = 0);

        void Start();

        void Stop();

        protected:

        frame_t frame;
        int frameLen;
        std::string inputFilePath;
        bool preReadingLock;
        bool readingLock;
        bool writingLock;

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

        std::set<void *> clients;
        std::set<void *> clientLocks;

        std::mutex lockMutex, clientsMutex, startStopMutex;
    };

}

#endif