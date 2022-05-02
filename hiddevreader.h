#ifndef _KMICKI_SDGYRODSU_HIDDEVREADER_H_
#define _KMICKI_SDGYRODSU_HIDDEVREADER_H_

#include <vector>
#include <string>
#include <thread>

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
        HidDevReader(int hidNo, int frameLen);

        // Get current frame data.
        // Locks frame for reading.
        // If frame is locked for writing, it waits until writing is finished.
        // After finishing reading the frame, it is improtant to call UnlockFrame().
        frame_t const& GetFrame();

        // Check if frame is currently being written to.
        // True means that GetFrame() will block until writing ends.
        bool const& IsFrameLockedForWriting();

        // Check if frame is locked for reading.
        // True means that new frame will not be generated.
        // Use UnlockFrame() to unlock it.
        bool const& IsFrameLockedForReading();

        // Lock frame for reading.
        void LockFrame();

        // Unlock frame.
        void UnlockFrame();

        // Destructor. 
        // Stops reading task.
        // Closes input file.
        ~HidDevReader();

        protected:

        frame_t frame;
        std::string inputFilePath;
        bool readingLock;
        bool writingLock;

        bool stopTask; // stop reading task

        std::unique_ptr<std::thread> readingTask;

        // Method called in a reading task on a separate thread.
        void executeReadingTask();
    };

}

#endif