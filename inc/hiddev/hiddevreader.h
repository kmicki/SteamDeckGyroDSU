#ifndef _KMICKI_HIDDEV_HIDDEVREADER_H_
#define _KMICKI_HIDDEV_HIDDEVREADER_H_

#include <vector>
#include <shared_mutex>

#include "pipeline/thread.h"
#include "pipeline/signalout.h"
#include "pipeline/pipeout.h"
#include "pipeline/serve.h"

using namespace kmicki::pipeline;

namespace kmicki::hiddev 
{
    void HandleMissedTicks(std::string name, std::string tickName, bool received, int & ticks, int period, int & nonMissed);

    // Reads periodic data from a given HID via HIDAPI
    // in constant-length frames and provides most recent frame.
    class HidDevReader
    {
        public:

        typedef std::vector<char> frame_t;

        HidDevReader() = delete;

        // Constructor.
        // Starts pipeline.
        // vId: vendor ID
        // pId: product ID
        // interfaceNumber: interface number of the device (obtained using hid_enumerate)
        // frameLen: Size of single HID data frame.
        // scanTimeUs: Period between frames in ms. Used to calculate timeout.
        HidDevReader(uint16_t const& vId, uint16_t const& pId, const int& interfaceNumber, int const& _frameLen, int const& scanTimeUs);

        // Destructor. 
        // Stops pipeline.
        // Closes input file.
        ~HidDevReader();

        // Get frame serve
        Serve<frame_t> & GetServe();

        // Stop serving
        void StopServe(Serve<frame_t> & serve);

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
        
        std::vector<std::unique_ptr<Thread>> pipeline;
        ReadData* readData;
        ReadDataApi* readDataApi;
        ServeFrame * serveFrame;

        // Mutex
        std::mutex startStopMutex;

        void AddOperation(pipeline::Thread * operation);
    };

}

#endif