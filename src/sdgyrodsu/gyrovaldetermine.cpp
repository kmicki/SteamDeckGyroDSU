#include "gyrovaldetermine.h"
#include <cmath>
#include <numeric>

#define ACCEL_1G 16500

namespace kmicki::sdgyrodsu
{
    GyroValDetermine::GyroValDetermine()
    : data(50000)
    {

    }

    void GyroValDetermine::Reset()
    {
        timeStart = 0;
        accelStart = 0;
        timeLast = 0;
        accelLast = 0;
        data.clear();
    }

    void GyroValDetermine::ProcessFrame(SdHidFrame const& frame)
    {
        if(timeStart == 0)
        {
            timeStart = frame.Increment;
            accelStart = frame.AccelAxisFrontToBack;
            return;
        }        

        timeLast = frame.Increment;
        accelLast = frame.AccelAxisFrontToBack;

        data.push_back(frame.GyroAxisRightToLeft);
    }

    long GyroValDetermine::GetDegPerSecond() const
    {
        long timeMs = (timeLast - timeStart)*4;
        double degLast = acos((double)accelLast / (double)ACCEL_1G) * (180.0/3.141592653589793238463);
        double degStart = acos((double)accelStart / (double)ACCEL_1G) * (180.0/3.141592653589793238463);

        double degs = degLast-degStart;

        double actDegsPerSecond = degs/((double)timeMs/1000.0);

        double count = (double)data.size();
        
        long sum=0;
        for(auto iter = data.begin();iter != data.end();++iter)
            sum+=*iter;
        double gyroDegsPerSecondAvg = (double)sum/count;

        double res = gyroDegsPerSecondAvg/actDegsPerSecond;
        return (long)res;
    }
}