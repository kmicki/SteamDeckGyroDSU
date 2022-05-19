#ifndef _KMICKI_SDGYRODSU_CEMUHOOKADAPTER_H_
#define _KMICKI_SDGYRODSU_CEMUHOOKADAPTER_H_

#include "sdhidframe.h"
#include "cemuhookprotocol.h"
#include "hiddevreader.h"

namespace kmicki::sdgyrodsu
{
    class CemuhookAdapter
    {
        public:
        CemuhookAdapter() = delete;

        CemuhookAdapter(hiddev::HidDevReader & _reader);

        void StartFrameGrab();
        cemuhook::protocol::MotionData const& GetMotionDataNewFrame();
        void StopFrameGrab();

        bool IsControllerConnected();

        cemuhook::protocol::MotionData GetMotionData(SdHidFrame const& frame, float &lastAccelRtL, float &lastAccelFtB, float &lastAccelTtB);
        static void SetMotionData(SdHidFrame const& frame, cemuhook::protocol::MotionData &data, float &lastAccelRtL, float &lastAccelFtB, float &lastAccelTtB);

        private:
        cemuhook::protocol::MotionData data;
        hiddev::HidDevReader & reader;

        uint32_t lastInc;
        
        float lastAccelRtL;
        float lastAccelFtB;
        float lastAccelTtB;

    };
}

#endif
