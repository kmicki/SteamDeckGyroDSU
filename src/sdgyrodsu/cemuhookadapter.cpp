#include "cemuhookadapter.h"
#include "sdhidframe.h"

#include <iostream>

using namespace kmicki::cemuhook::protocol;

#define SD_SCANTIME_US 4000
#define ACC_1G 0x4000
#define GYRO_1DEGPERSEC 16
#define GYRO_DEADZONE 8
#define ACCEL_SMOOTH 0x1FF

namespace kmicki::sdgyrodsu
{

    MotionData GetMotionData(SdHidFrame const& frame)
    {
        MotionData data;

        SetMotionData(frame,data);

        return data;
    }
    
    float SmoothAccel(float &last, int16_t curr)
    {
        static const float acc1G = (float)ACC_1G;
        if(abs(curr - last) < ACCEL_SMOOTH)
        {
            last = ((float)last*0.95+(float)curr*0.05);
        }
        else
        {
            last = (float)curr;
        }
        return last/acc1G;
    }

    void SetMotionData(SdHidFrame const& frame, MotionData &data)
    {
        static const float acc1G = (float)ACC_1G;
        static const float gyro1dps = (float)GYRO_1DEGPERSEC;

        uint64_t timestamp = (uint64_t)frame.Increment*SD_SCANTIME_US;

        data.timestampL = (uint32_t)(timestamp & 0xFFFFFFFF);
        data.timestampH = (uint32_t)(timestamp >> 32);

        static float lastAccelRtL = 0;
        static float lastAccelFtB = 0;
        static float lastAccelTtB = 0;
        
        data.accX = -SmoothAccel(lastAccelRtL,frame.AccelAxisRightToLeft);
        data.accY = -SmoothAccel(lastAccelFtB,frame.AccelAxisFrontToBack);
        data.accZ = SmoothAccel(lastAccelTtB,frame.AccelAxisTopToBottom);
        if(frame.Header & 0xFF == 0xDD)
        {
            data.pitch = 0.0f;
            data.yaw = 0.0f;
            data.roll = 0.0f;
        }
        else 
        {
            auto gyroRtL = frame.GyroAxisRightToLeft;
            auto gyroFtB = frame.GyroAxisFrontToBack;
            auto gyroTtB = frame.GyroAxisTopToBottom;

            if(gyroRtL < GYRO_DEADZONE && gyroRtL > -GYRO_DEADZONE)
                gyroRtL = 0;
            if(gyroFtB < GYRO_DEADZONE && gyroFtB > -GYRO_DEADZONE)
                gyroFtB = 0;
            if(gyroTtB < GYRO_DEADZONE && gyroTtB > -GYRO_DEADZONE)
                gyroTtB = 0;

            data.pitch = (float)gyroRtL/gyro1dps;
            data.yaw = -(float)gyroFtB/gyro1dps;
            data.roll = (float)gyroTtB/gyro1dps;
        }
    }

    CemuhookAdapter::CemuhookAdapter(hiddev::HidDevReader & _reader)
    : reader(_reader)
    {
        ;
    }

    void CemuhookAdapter::StartFrameGrab()
    {
        reader.Start();
    }

    MotionData const& CemuhookAdapter::GetMotionDataNewFrame()
    {
        static uint32_t lastInc = 0;
        while(true)
        {
            auto const& frame = reader.GetNewFrame();

            auto const& inc = *reinterpret_cast<uint32_t const*>(frame.data()+4);

            if(inc <= lastInc && lastInc-inc < 40)
            {
                reader.UnlockFrame();
            }
            else
            {
                lastInc = inc;

                SetMotionData(GetSdFrame(frame),data);
                reader.UnlockFrame();
                return data;
            }
        }
    }

    void CemuhookAdapter::StopFrameGrab()
    {
        reader.Stop();
    }

    bool CemuhookAdapter::IsControllerConnected()
    {
        return true;
    }
}