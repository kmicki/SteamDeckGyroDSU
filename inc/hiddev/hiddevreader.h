#ifndef _KMICKI_HIDDEV_HIDDEVREADER_H_
#define _KMICKI_HIDDEV_HIDDEVREADER_H_

#include <vector>
#include <fstream>
#include <chrono>
#include <mutex>
#include <shared_mutex>

#include "pipeline/thread.h"
#include "pipeline/signalout.h"
#include "pipeline/pipeout.h"
#include "pipeline/serve.h"

using namespace kmicki::pipeline;

namespace kmicki::hiddev 
{
    void HandleMissedTicks(std::string name, std::string tickName, bool received, int & ticks, int period, int & nonMissed);

    // Reads periodic data from a given HID device (/dev/usb/hiddevX)
    // in constant-length frames and provides most recent frame.
    class HidDevReader
    {
        public:

        typedef std::vector<char> frame_t;

        HidDevReader() = delete;

        // Constructor.
        // Starts pipeline.
        // hidNo: ID of HID device (X in /dev/usb/hiddevX)
        // frameLen: Size of single HID data frame
        // scanTime: Period between frames in ms. 
        //           If it will be around or lower than actual period of incoming frames,
        //           The reading task will block often and will have to reinitialize reading.
        //           If it will be much higher then the generated frames will be out of sync
        //           (a block of consecutive frames and then skip)
        // maxScanTime: maximum scan time
        HidDevReader(int hidNo, int _frameLen);

        // Destructor. 
        // Stops pipeline.
        // Closes input file.
        ~HidDevReader();

        // Set start marker in case hiddev file doesn't provide a way to find start of the frame.
        void SetStartMarker(std::vector<char> const& marker);

        // Get frame serve
        Serve<frame_t> & GetServe();

        // Stop serving
        void StopServe(Serve<frame_t> & _serve);

        // Start process of grabbing frames.
        void Start();

        // Stop process of grabbing frames
        void Stop();

        // Is grabbing frames in progress?
        bool IsStarted();

        // Is grabbing frames being stopped right now?
        bool IsStopping();

        private:

        // Pipeline threads

        class ReadData : public Thread
        {
            public:
            ReadData() = delete;
            ReadData(std::string const& _inputFilePath, int const& _frameLen);//, SignalOut & _tick);
            ~ReadData();

            void ReconnectInput();
            void DisconnectInput();

            void SetStartMarker(std::vector<char> const& marker); 

            PipeOut<std::vector<char>> Data;
            SignalOut Unsynced;

            protected:

            void Execute() override;
            void FlushPipes() override;

            private:
            bool CheckData(std::unique_ptr<std::vector<char>> const& data);
            std::ifstream inputStream;
            std::string inputFilePath;
            std::vector<char> startMarker;
        };

        class ProcessData : public Thread
        {
            public:
            ProcessData() = delete;
            ProcessData(int const& _frameLen, ReadData & _data);
            ~ProcessData();

            PipeOut<frame_t> Frame;
            SignalOut ReadStuck;

            protected:

            void Execute() override;
            void FlushPipes() override;

            private:
            ReadData & readData;
            PipeOut<std::vector<char>> & data;
        };

        class ServeFrame : public Thread
        {
            public:
            ServeFrame() = delete;
            ServeFrame(PipeOut<frame_t> & _frame);
            ~ServeFrame();

            Serve<frame_t> & GetServe();

            void StopServe(Serve<frame_t> & serve);

            protected:

            void Execute() override;
            void FlushPipes() override;

            private:
            void WaitForServes();
            void HandleMissedFrames(int &serveCnt, std::vector<int> &missedTicks, std::vector<int> &nonMissedTicks, std::vector<std::string> &serveNames);
            PipeOut<frame_t> & frame;
            std::vector<std::unique_ptr<Serve<frame_t>>> frames;
            std::vector<Serve<frame_t>::ServeLock> GetServeLocks();
            std::shared_mutex framesMutex;
            std::condition_variable_any framesCv;
        };

        static const int cInputRecordLen;
        static const int cByteposInput;

        int frameLen;

        std::string inputFilePath;
        
        std::vector<std::unique_ptr<Thread>> pipeline;
        ServeFrame * serve;
        ReadData* read;

        // Mutex
        std::mutex startStopMutex;

        void AddOperation(pipeline::Thread * operation);

        
    };

}

#endif