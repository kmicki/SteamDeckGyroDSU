#ifndef _KMICKI_SDGYRODSU_HIDDEVREADER_H_
#define _KMICKI_SDGYRODSU_HIDDEVREADER_H_

#include <vector>
#include <string>
#include <thread>
#include <fstream>

namespace kmicki::sdgyrodsu 
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
        frame_t const& GetFrame();

        // Check if frame is currently being written to.
        // True means that GetFrame() will block until writing ends.
        bool const& IsFrameLockedForWriting() const;

        // Check if frame is locked for reading.
        // True means that new frame will not be generated.
        // Use UnlockFrame() to unlock it.
        bool const& IsFrameLockedForReading() const;

        // Lock frame for reading.
        // If frame is locked for writing, it waits until writing is finished.
        void LockFrame();

        // Unlock frame.
        void UnlockFrame();

        protected:

        frame_t frame;
        int frameLen;
        std::string inputFilePath;
        bool preReadingLock;
        bool readingLock;
        bool writingLock;

        std::chrono::milliseconds scanPeriod;

        bool stopTask; // stop reading task

        std::unique_ptr<std::thread> readingTask;

        // Method called in a reading task on a separate thread.
        void executeReadingTask();

        static void reconnectInput(std::ifstream & stream, std::string path);
        static void processData(std::vector<char> const& bufIn, frame_t &bufOut);        

        std::unique_ptr<std::ifstream> inputStream;
    };

}

#endif