#include "sdgyrodsu/cemuhookadapter.h"
#include "sdgyrodsu/sdhidframe.h"
#include "log/log.h"

#include <iostream>
#include <iomanip>

using namespace kmicki::cemuhook::protocol;
using namespace kmicki::log;

#define SD_SCANTIME_US 4000
#define ACC_1G 0x4000
#define GYRO_1DEGPERSEC 16
#define GYRO_DEADZONE 8
#define ACCEL_SMOOTH 0x1FF

namespace kmicki::sdgyrodsu
{

    MotionData CemuhookAdapter::GetMotionData(SdHidFrame const& frame, float &lastAccelRtL, float &lastAccelFtB, float &lastAccelTtB)
    {
        MotionData data;

        SetMotionData(frame,data,lastAccelRtL,lastAccelFtB,lastAccelTtB);

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

    MotionData & SetTimestamp(MotionData &data, uint64_t const& timestamp)
    {
        data.timestampL = (uint32_t)(timestamp & 0xFFFFFFFF);
        data.timestampH = (uint32_t)(timestamp >> 32);

        return data;
    }

    uint64_t ToTimestamp(uint32_t const& increment)
    {
        return (uint64_t)increment*SD_SCANTIME_US;
    }

    MotionData &  SetTimestamp(MotionData &data, uint32_t const& increment)
    {
        SetTimestamp(data,ToTimestamp(increment));

        return data;
    }

    void CemuhookAdapter::SetMotionData(SdHidFrame const& frame, MotionData &data, float &lastAccelRtL, float &lastAccelFtB, float &lastAccelTtB)
    {
        static const float acc1G = (float)ACC_1G;
        static const float gyro1dps = (float)GYRO_1DEGPERSEC;

        SetTimestamp(data, frame.Increment);
        
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

    CemuhookAdapter::CemuhookAdapter(hiddev::HidDevReader & _reader, bool persistent)
    : reader(_reader),
      lastInc(0),
      lastAccelRtL(0.0),lastAccelFtB(0.0),lastAccelTtB(0.0),
      isPersistent(persistent), toReplicate(0), noGyroCooldown(0)
    {
        Log("CemuhookAdapter: Initialized. Waiting for start of frame grab.",LogLevelDebug);
    }

    void CemuhookAdapter::StartFrameGrab()
    {
        lastInc = 0;
        ignoreFirst = true;
        Log("CemuhookAdapter: Starting frame grab.",LogLevelDebug);
        reader.Start();
        frameServe = &reader.GetServe();
    }

    int const& CemuhookAdapter::SetMotionDataNewFrame(MotionData &motion)
    {
        static const int64_t cMaxDiffReplicate = 100;
        static const int cNoGyroCooldownFrames = 1000;
        static const int cMaxRepeatedLoop = 1000;

        if(noGyroCooldown > 0) --noGyroCooldown;

        auto const& dataFrame = frameServe->GetPointer();

        if(ignoreFirst)
        {
            auto lock = frameServe->GetConsumeLock();
            ignoreFirst = false;
        }

        int repeatedLoop = cMaxRepeatedLoop;

        while(true)
        {
            if(toReplicate == 0)
            {
                //Log("DEBUG: TRY GET CONSUME LOCK.");
                auto lock = frameServe->GetConsumeLock();
                //Log("CONSUME LOCK ACQUIRED.");
                auto const& frame = GetSdFrame(*dataFrame);

                if( noGyroCooldown <= 0
                    &&  frame.AccelAxisFrontToBack == 0 && frame.AccelAxisRightToLeft == 0 
                    &&  frame.AccelAxisTopToBottom == 0 && frame.GyroAxisFrontToBack == 0 
                    &&  frame.GyroAxisRightToLeft == 0 && frame.GyroAxisTopToBottom == 0)
                {
                    NoGyro.SendSignal();
                    noGyroCooldown = cNoGyroCooldownFrames;
                }

                int64_t diff = (int64_t)frame.Increment - (int64_t)lastInc;

                if(lastInc != 0 && diff < 1 && diff > -100)
                {
                    if(repeatedLoop == cMaxRepeatedLoop)
                    {
                        Log("CemuhookAdapter: Frame was repeated. Ignoring...",LogLevelDebug);
                        { LogF(LogLevelTrace) << std::setw(8) << std::setfill('0') << std::setbase(16)
                                        << "Current increment: 0x" << frame.Increment << ". Last: 0x" << lastInc << "."; }
                    }
                    if(repeatedLoop <= 0)
                    {
                        Log("CemuhookAdapter: Frame is repeated continously...");
                        return toReplicate;
                    }
                    --repeatedLoop;
                }
                else
                {
                    if(lastInc != 0 && diff > 1)
                    {
                        LogF logMsg((diff > 6)?LogLevelDefault:LogLevelDebug);
                        logMsg << "CemuhookAdapter: Missed " << (diff-1) << " frames.";
                        if(diff > 1000)
                            { LogF(LogLevelTrace) << std::setw(8) << std::setfill('0') << std::setbase(16)
                                     << "Current increment: 0x" << frame.Increment << ". Last: 0x" << lastInc << "."; }
                        if(diff <= cMaxDiffReplicate)
                        {
                            logMsg << " Replicating...";
                            toReplicate = diff-1;
                        }
                    }

                    SetMotionData(frame,motion,lastAccelRtL,lastAccelFtB,lastAccelTtB);

                    if(toReplicate > 0)
                    {
                        lastTimestamp = ToTimestamp(lastInc+1);
                        SetTimestamp(motion,lastTimestamp);
                        if(!isPersistent)
                            data = motion;
                    }
                        
                    lastInc = frame.Increment;
                    
                    return toReplicate;
                }
            }
            else
            {
                // Replicated frame
                --toReplicate;
                lastTimestamp += SD_SCANTIME_US;
                if(!isPersistent)
                {
                    motion = SetTimestamp(data,lastTimestamp);
                }
                else
                    SetTimestamp(motion,lastTimestamp);

                return toReplicate;
            }
        }
    }

    void CemuhookAdapter::StopFrameGrab()
    {
        Log("CemuhookAdapter: Stopping frame grab.",LogLevelDebug);
        reader.StopServe(*frameServe);
        frameServe = nullptr;
        reader.Stop();
    }

    bool CemuhookAdapter::IsControllerConnected()
    {
        return true;
    }
}