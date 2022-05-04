#ifndef _KMICKI_SDGYRODSU_CEMUHOOKADAPTER_H_
#define _KMICKI_SDGYRODSU_CEMUHOOKADAPTER_H_

#include "sdhidframe.h"
#include "cemuhookprotocol.h"

namespace kmicki::sdgyrodsu
{
    cemuhook::protocol::MotionData GetMotionData(SdHidFrame const& frame);
    void SetMotionData(SdHidFrame const& frame, cemuhook::protocol::MotionData &data);
}

#endif
