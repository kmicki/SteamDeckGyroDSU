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
        static void Present(SdHidFrame const& frame);
        static void Finish();
    };
}

#endif