#ifndef _KMICKI_SDGYRODSU_PRESENTER_H_
#define _KMICKI_SDGYRODSU_PRESENTER_H_

#include <vector>
#include "sdhidframe.h"

namespace kmicki::sdgyrodsu
{
    class Presenter
    {
        public:
        static void Initialize();
        static void Present(frame_t frame);
        static void Finish();
    };
}

#endif