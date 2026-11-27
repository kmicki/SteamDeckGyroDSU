#include "hiddev/hiddevreader.h"
#include "hiddev/iioimu.h"
#include "log/log.h"

#include <cmath>
#include <cstring>
#include <thread>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    // Steam Deck HID frame layout and units
    static const char cFrameHeader[4] = { 0x01, 0x00, 0x09, 0x40 };
    static const int cFrameIncrementPos = 4;
    static const int cFrameAccelPos = 24;
    static const int cFrameGyroPos = 30;
    static const double cAccel1G = 16384.0;         // Steam Deck accelerometer units per 1g
    static const double cGyro1DegPerSec = 16.0;     // Steam Deck gyroscope units per 1 deg/s
    static const double cStandardGravity = 9.80665; // m/s^2
    static const double cRadToDeg = 180.0 / M_PI;

    static const auto cReopenPeriod = std::chrono::seconds(1);

    static int16_t ToInt16(double value)
    {
        value = std::round(value);
        if(value > INT16_MAX) return INT16_MAX;
        if(value < INT16_MIN) return INT16_MIN;
        return (int16_t)value;
    }

    static void PutInt16(std::vector<char> &frame, int pos, int16_t value)
    {
        frame[pos] = (char)(value & 0xFF);
        frame[pos+1] = (char)((value >> 8) & 0xFF);
    }

    HidDevReader::ReadDataIio::ReadDataIio(IioImuConfig const& _config, int const& _frameLen, int const& _scanTimeUs)
    : ReadData(_frameLen), config(_config), scanTime(_scanTimeUs)
    { }

    HidDevReader::ReadDataIio::~ReadDataIio()
    {
        TryStopThenKill();
    }

    void HidDevReader::ReadDataIio::Execute()
    {
        IioImu imu(config.devicePath);

        Log("HidDevReader::ReadDataIio: Opening IIO IMU device.",LogLevelDebug);
        bool reportedOpenFail = false;
        while(!imu.Open())
        {
            if(!reportedOpenFail)
            {
                { LogF() << "HidDevReader::ReadDataIio: Problem opening IIO IMU device at " << config.devicePath << ". Retrying..."; }
                reportedOpenFail = true;
            }
            std::this_thread::sleep_for(cReopenPeriod);
            if(!ShouldContinue())
                return;
        }

        auto const& data = Data.GetPointerToFill();

        // Last read sensor values in sensor axes.
        // Accelerometer axes are read in a round-robin manner, one every other frame,
        // because each sysfs read is a separate bus transaction
        // and reading all 6 values does not fit in a single frame period.
        // Gravity changes slowly and accelerometer values are smoothed later anyway.
        double accel[3] = { 0.0, 0.0, 0.0 };
        double gyro[3] = { 0.0, 0.0, 0.0 };

        for(int i = 0; i < 3; ++i)
            imu.ReadAccel(i,accel[i]);

        Log("HidDevReader::ReadDataIio: Started.",LogLevelDebug);

        auto start = std::chrono::steady_clock::now();
        uint64_t tick = 0;
        uint32_t increment = 1;
        int lateFrames = 0;

        while(ShouldContinue())
        {
            auto next = start + scanTime*tick;
            std::this_thread::sleep_until(next);

            if(!ShouldContinue())
                break;

            bool ok = true;
            for(int i = 0; i < 3 && ok; ++i)
                ok = imu.ReadGyro(i,gyro[i]);
            if(ok && tick % 2 == 0)
            {
                int axis = (tick / 2) % 3;
                ok = imu.ReadAccel(axis,accel[axis]);
            }

            if(!ok)
            {
                Log("HidDevReader::ReadDataIio: Problem reading IIO IMU device. Reopening...");
                imu.Close();
                while(!imu.Open())
                {
                    std::this_thread::sleep_for(cReopenPeriod);
                    if(!ShouldContinue())
                        return;
                }
                start = std::chrono::steady_clock::now();
                increment += 1000; // mark discontinuity
                tick = 0;
                continue;
            }

            auto &frame = *data;
            std::fill(frame.begin(),frame.end(),0);
            std::memcpy(frame.data(),cFrameHeader,sizeof(cFrameHeader));
            std::memcpy(frame.data()+cFrameIncrementPos,&increment,sizeof(increment));

            for(int i = 0; i < 3; ++i)
            {
                auto const& a = config.accel[i];
                auto const& g = config.gyro[i];
                PutInt16(frame,cFrameAccelPos+2*i,ToInt16(a.sign*accel[a.source]/cStandardGravity*cAccel1G));
                PutInt16(frame,cFrameGyroPos+2*i,ToInt16(g.sign*gyro[g.source]*cRadToDeg*cGyro1DegPerSec));
            }

            Data.SendData();

            // Advance to the next tick. If reading took too long, skip ticks
            // so that the increment (used as a timestamp) follows real time.
            auto now = std::chrono::steady_clock::now();
            uint64_t nextTick = tick + 1;
            uint64_t nowTick = (now - start) / scanTime;
            if(nowTick >= nextTick)
            {
                ++lateFrames;
                nextTick = nowTick + 1;
                if(lateFrames % 1000 == 1)
                    { LogF(LogLevelDebug) << "HidDevReader::ReadDataIio: Reading IMU is slower than frame period. Late frames: " << lateFrames << "."; }
            }
            increment += (uint32_t)(nextTick - tick);
            tick = nextTick;
        }

        imu.Close();
        Log("HidDevReader::ReadDataIio: Stopped.",LogLevelDebug);
    }
}
