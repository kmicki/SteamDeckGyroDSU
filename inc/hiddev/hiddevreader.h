#ifndef _KMICKI_HIDDEV_HIDDEVREADER_H_
#define _KMICKI_HIDDEV_HIDDEVREADER_H_

#include <vector>
#include <mutex>
#include <shared_mutex>
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
        // client parameter allows to get frame and lock for many clients
        frame_t const& GetFrame(void* client = nullptr);

        // Get current frame data without locking for reading.
        frame_t const& Frame();

        // Get current frame data.
        // Locks frame for reading.
        // If frame is locked for writing, it waits until writing is finished.
        // Blocks if there was no new frame after last use of GetFrame or GetNewFrame
        frame_t const& GetNewFrame(void* client = nullptr);

        // Unlock frame.
        void UnlockFrame(void* client = nullptr);

        // Start process of grabbing frames.
        void Start();

        // Stop process of grabbing frames
        void Stop();

        // Is grabbing frames in progress?
        bool IsStarted();

        // Is grabbing frames being stopped right now?
        bool IsStopping();

        private:

        static const int cInputRecordLen;

        frame_t frame;
        int frameLen;

        std::string inputFilePath;

        std::chrono::microseconds scanPeriod;   // set time span between read attempts
        
        // Threads
        std::unique_ptr<std::thread> readingTask;

        // Stop tasks
        bool stopTask;  // stop frame grab task
        bool stopRead;  // stop data read task
        bool stopMetro; // stop metronome

        // Mutexes
        std::mutex mainMutex;                           // stop entire task
        std::shared_mutex frameMutex;                   // reading and writing to frame
        std::shared_mutex clientDeliveredMutex;         // access to clients delivered vector
        std::mutex startStopMutex;                      // starting/stopping frame grab
        std::mutex readTaskMutex;                       // stopping read task/reading data from HID
        std::mutex stopMetroMutex;                      // stopping metronome
        std::mutex tickMutex;                           // metronome tick

        // Condition variables
        std::condition_variable readTaskProceed; 
        std::condition_variable frameProceed;
        std::condition_variable_any newFrameProceed;
        std::condition_variable_any nextFrameProceed;

        // Separate tasks for separate threads
        void mainTask();                                // main task run in a thread
        void readTask(std::unique_ptr<std::vector<char>> const& buf);         // read from hiddev
        void Metronome();                               // pace the readTask reading
        
        // Flow control readTask-Metronome-mainTask
        bool readTick,hidDataReady;
        
        // Stream for reading from hiddev
        std::unique_ptr<std::ifstream> inputStream;

        // helper functions
        static void reconnectInput(std::ifstream & stream, std::string path);   // open hiddev file again
        void processData(std::vector<char> const& bufIn);                       // convert raw hiddevN data into HID frame
        bool LossAnalysis(uint32_t const& diff);                                      // analyze lost packets and adjust scan time
        void IntializeLossAnalysis();   // intialize fields

        // Fields used by loss analysis
        int lossPeriod;
        bool lossAnalysis;
        uint32_t lastInc;
        bool smallLossEncountered;
        std::chrono::microseconds bigLossDuration;
        std::chrono::system_clock::time_point lastAnalyzedFrameLossTime;
        bool passedNoLossAnalysis;
        int readStuck;

        // Clients reading frame data
        std::vector<void*> frameLockClients;        // clients that acquired frame data but didn't signal unlock yet
        std::vector<void*> frameDeliveredClients;   // clients that already had the current frame delivered
    };

}

#endif