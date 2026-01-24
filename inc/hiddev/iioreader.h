#ifndef _KMICKI_HIDDEV_IIOREADER_H_
#define _KMICKI_HIDDEV_IIOREADER_H_

#include <string>
#include <vector>
#include <fstream>
#include "pipeline/thread.h"
#include "pipeline/serve.h"
#include "pipeline/signalout.h"

#include "hiddevreader.h"

namespace kmicki::hiddev
{
    class IioReader : public pipeline::Thread, public MotionReader
    {
    public:
        typedef std::vector<char> frame_t;

        IioReader(int scanTimeUs);
        virtual ~IioReader();

        pipeline::Serve<frame_t> & GetServe() override;
        void Start() override;
        void Stop() override;
        bool IsStarted() override;
        void SetNoGyro(pipeline::SignalOut& _noGyro) override;
        void SetStartMarker(std::vector<char> const& marker) override;
        void StopServe(pipeline::Serve<frame_t> & _serve) override;

    protected:
        void Execute() override;
        void FlushPipes() override;

    private:
        int scanTimeUs;
        pipeline::Serve<frame_t> frameServe;
        std::unique_ptr<frame_t> frame;
        bool started;

        std::string accelPath;
        std::string gyroPath;
        
        float accelScale[3];
        float gyroScale[3];

        std::string accelRawPaths[3];
        std::string gyroRawPaths[3];

        bool hasAccel;
        bool hasGyro;

        bool FindDevices();
        float ReadFloat(std::string const& path);
        int64_t ReadInt(std::string const& path);
        bool FileExists(std::string const& path);
    };
}

#endif
