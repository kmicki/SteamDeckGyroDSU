#ifndef _KMICKI_SDGYRODSU_CEMUHOOKADAPTER_H_
#define _KMICKI_SDGYRODSU_CEMUHOOKADAPTER_H_

#include "sdhidframe.h"
#include "cemuhookprotocol.h"
#include "hiddevreader.h"

namespace kmicki::sdgyrodsu
{
    cemuhook::protocol::MotionData GetMotionData(SdHidFrame const& frame);
    void SetMotionData(SdHidFrame const& frame, cemuhook::protocol::MotionData &data);

    class CemuhookAdapter
    {
        public:
        CemuhookAdapter() = delete;

        CemuhookAdapter(hiddev::HidDevReader & _reader);

        void StartFrameGrab();
        cemuhook::protocol::MotionData const& GetMotionDataNewFrame();
        void StopFrameGrab();

        bool IsControllerConnected();

        private:
        cemuhook::protocol::MotionData data;
        hiddev::HidDevReader & reader;

    };
}

#endif
