#ifndef _KMICKI_HIDDEV_HIDDEVREADER_H_
#define _KMICKI_HIDDEV_HIDDEVREADER_H_

#include <vector>
#include <shared_mutex>

#include "pipeline/thread.h"
#include "pipeline/signalout.h"
#include "pipeline/pipeout.h"
#include "pipeline/serve.h"

#include "hiddevfile.h"

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
        // Uses hiddev file to obtain data from device.
        // hidNo: ID of HID device (X in /dev/usb/hiddevX)
        // frameLen: Size of single HID data frame
        // scanTime: Period between frames in ms. 
        //           If it will be around or lower than actual period of incoming frames,
        //           The reading task will block often and will have to reinitialize reading.
        //           If it will be much higher then the generated frames will be out of sync
        //           (a block of consecutive frames and then skip)
        // maxScanTime: maximum scan time
        HidDevReader(int const& hidNo, int const& _frameLen, int const& scanTimeUs);

        // Constructor.
        // Starts pipeline.
        // Uses hidapi (hidraw) to obtain data from device.
        // vId: vendor ID
        // pId: product ID
        // interfaceNumber: interface number of the device
        // frameLen: Size of single HID data frame
        // scanTime: Period between frames in ms. 
        //           If it will be around or lower than actual period of incoming frames,
        //           The reading task will block often and will have to reinitialize reading.
        //           If it will be much higher then the generated frames will be out of sync
        //           (a block of consecutive frames and then skip)
        // maxScanTime: maximum scan time
        HidDevReader(uint16_t const& vId, uint16_t const& pId, const int& interfaceNumber, int const& _frameLen, int const& scanTimeUs);

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

        void SetNoGyro(SignalOut& _noGyro);

        private:

        // Pipeline threads

        class ReadData : public Thread
        {
            public:
            ReadData() = delete;
            ReadData(int const& _frameLen);
            ~ReadData();

            void SetStartMarker(std::vector<char> const& marker);

            PipeOut<std::vector<char>> Data;
            SignalOut Unsynced;

            protected:

            void FlushPipes() override;
            std::vector<char> startMarker;
        };

        class ReadDataFile : public ReadData
        {
            public:
            ReadDataFile() = delete;
            ReadDataFile(std::string const& _inputFilePath, int const& _frameLen, int const& _scanTimeUs);

            void ReconnectInput();
            void DisconnectInput();

            protected:

            void Execute() override;

            private:
            bool CheckData(std::unique_ptr<std::vector<char>> const& data, ssize_t readCnt);
            HidDevFile inputFile;
        };
        
        class ReadDataApi : public ReadData
        {
            public:
            ReadDataApi() = delete;
            ReadDataApi(uint16_t const& vId, uint16_t const& pId, const int& _interfaceNumber, int const& _frameLen, int const& _scanTimeUs);
            ~ReadDataApi();

            void SetNoGyro(SignalOut& _noGyro);

            protected:

            void Execute() override;

            private:
            uint16_t vId;
            uint16_t pId;
            int interfaceNumber;
            int timeout;

            SignalOut *noGyro;
        };

        class ProcessData : public Thread
        {
            public:
            ProcessData() = delete;
            ProcessData(int const& _frameLen, ReadData & _data, int const& scanTimeUs);
            ~ProcessData();

            PipeOut<frame_t> Frame;
            SignalOut ReadStuck;

            protected:

            void Execute() override;
            void FlushPipes() override;

            private:
            ReadData & readData;
            PipeOut<std::vector<char>> & data;

            std::chrono::microseconds timeout;
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
        ReadData* readData;
        ReadDataApi* readDataApi;

        // Mutex
        std::mutex startStopMutex;

        void AddOperation(pipeline::Thread * operation);

        void ConstructPipeline(ReadData* _readData, int const& _frameLen, int const& scanTimeUs, bool useProcessData = true);

        
    };

}

#endif