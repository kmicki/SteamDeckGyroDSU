#ifndef _KMICKI_SDGYRODSU_GYROVALDETERMINE_H_
#define _KMICKI_SDGYRODSU_GYROVALDETERMINE_H_

#include "sdhidframe.h"
#include <vector>
#include <cstdint>

namespace kmicki::sdgyrodsu
{

    class GyroValDetermine
    {
        

        public:
        GyroValDetermine();
        void Reset();
        void ProcessFrame(SdHidFrame const& frame);
        long GetDegPerSecond() const;

        private:

        int timeStart;
        int timeLast;
        int accelStart;
        int accelLast;
        std::vector<int16_t> data;
    };

}


#endif