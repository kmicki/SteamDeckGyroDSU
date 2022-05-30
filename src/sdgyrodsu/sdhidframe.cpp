#include "sdgyrodsu/sdhidframe.h"

namespace kmicki::sdgyrodsu
{
    SdHidFrame const& GetSdFrame(frame_t const& frame)
    {
        return *reinterpret_cast<SdHidFrame const*>(frame.data());
    }
}