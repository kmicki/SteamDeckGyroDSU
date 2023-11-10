#ifndef _KMICKI_SDGYRODSU_CEMUHOOKADAPTER_H_
#define _KMICKI_SDGYRODSU_CEMUHOOKADAPTER_H_

#include "sdhidframe.h"
#include "cemuhook/cemuhookprotocol.h"
#include "hiddev/hiddevreader.h"
#include "pipeline/serve.h"
#include "pipeline/signalout.h"

namespace kmicki::sdgyrodsu
{
    class CemuhookAdapter
    {
        public:
        CemuhookAdapter() = delete;

        CemuhookAdapter(hiddev::HidDevReader & _reader, bool persistent = true);

        void StartFrameGrab();

        // Modifies motion data in place.
        // Returns number if frames to be replicated in next calls (in case of missing frames).
        // persistent: true when motion structure is not modified between calls.
        int const& SetMotionDataNewFrame(cemuhook::protocol::MotionData &motion);
        void StopFrameGrab();

        bool IsControllerConnected();

        cemuhook::protocol::MotionData GetMotionData(SdHidFrame const& frame, float &lastAccelRtL, float &lastAccelFtB, float &lastAccelTtB);
        static void SetMotionData(SdHidFrame const& frame, cemuhook::protocol::MotionData &data, float &lastAccelRtL, float &lastAccelFtB, float &lastAccelTtB);

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

        pipeline::Serve<hiddev::HidDevReader::frame_t> * frameServe;
    };
}

#endif
