#include "cemuhook/sdcontroller/hidframe.h"

namespace kmicki::cemuhook::sdcontroller
{
    HidFrame const& GetSdFrame(frame_t const& frame)
    {
        return *reinterpret_cast<HidFrame const*>(frame.data());
    }
}