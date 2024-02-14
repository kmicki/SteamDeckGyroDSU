#ifndef _KMICKI_CEMUHOOK_SDCONTROLLER_DATASOURCE_H_
#define _KMICKI_CEMUHOOK_SDCONTROLLER_DATASOURCE_H_

#include "hidframe.h"
#include "cemuhook/cemuhookprotocol.h"
#include "hiddev/hiddevreader.h"
#include "pipeline/serve.h"
#include "pipeline/signalout.h"
#include "hiddev/hiddev.h"
#include "../datasource.h"

namespace kmicki::cemuhook::sdcontroller
{
    // TODO: abstract out the DataSource interface
    // TODO: make an adapter a pipeline thread
    // TODO: connect pipe to WriteData to send gyro enable frame
    class DataSource : public kmicki::cemuhook::DataSource
    {
        public:
        DataSource() = delete;

        DataSource(hiddev::HidDevReader & _reader, bool persistent = true);

        virtual void Start();

        // Modifies controller data in place.
        // Returns number of frames to be replicated in next calls (in case of missing frames).
        virtual int const& SetDataNewFrame(cemuhook::protocol::MotionData &motion);
        virtual void Stop();

        virtual bool IsControllerConnected();

        cemuhook::protocol::MotionData GetMotionData(HidFrame const& frame, float &lastAccelRtL, float &lastAccelFtB, float &lastAccelTtB);
        static void SetMotionData(HidFrame const& frame, cemuhook::protocol::MotionData &data, float &lastAccelRtL, float &lastAccelFtB, float &lastAccelTtB);

        SignalOut NoGyro;

        private:
        bool ignoreFirst;
        bool isPersistent;

        cemuhook::protocol::MotionData data;
        hiddev::HidDevReader & reader;

        uint32_t lastInc;
        uint64_t lastTimestamp;
        
        float lastAccelRtL;
        float lastAccelFtB;
        float lastAccelTtB;

        int toReplicate;
        int noGyroCooldown;

        pipeline::Serve<frame_t> * frameServe;
    };
}

#endif
