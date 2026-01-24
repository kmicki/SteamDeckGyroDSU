#include "hiddev/iioreader.h"
#include "sdgyrodsu/sdhidframe.h"
#include "log/log.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <memory>

namespace fs = std::filesystem;

namespace kmicki::hiddev
{
    IioReader::IioReader(int _scanTimeUs)
        : scanTimeUs(_scanTimeUs), frame(new frame_t(sizeof(sdgyrodsu::SdHidFrame))), started(false),
          hasAccel(false), hasGyro(false)
    {
        frameServe.SetObject(frame);
        if (FindDevices()) {
            log::Log("IioReader: Devices found.");
        } else {
            log::Log("IioReader: No compatible IIO devices found.");
        }
    }

    IioReader::~IioReader()
    {
        Stop();
    }

    pipeline::Serve<IioReader::frame_t> & IioReader::GetServe()
    {
        return frameServe;
    }

    void IioReader::Start()
    {
        if (started) return;
        Thread::Start();
        started = true;
    }

    void IioReader::Stop()
    {
        if (!started) return;
        Thread::Stop();
        started = false;
    }

    void IioReader::StopServe(pipeline::Serve<frame_t>& /*_serve*/)
    {
        // Intentionally left blank:
        // pipeline::Serve has no explicit Stop()/Close() in this codebase.
        // Stopping is handled via IioReader::Stop() / thread shutdown.
    }

    bool IioReader::IsStarted()
    {
        return started;
    }

    void IioReader::SetNoGyro(pipeline::SignalOut& _noGyro)
    {
    }

    void IioReader::SetStartMarker(std::vector<char> const& marker)
    {
    }

    bool IioReader::FileExists(std::string const& path)
    {
        return fs::exists(path);
    }

    float IioReader::ReadFloat(std::string const& path)
    {
        std::ifstream file(path);
        float val = 0.0f;
        if (file >> val) return val;
        return 0.0f;
    }

    int64_t IioReader::ReadInt(std::string const& path)
    {
        std::ifstream file(path);
        int64_t val = 0;
        if (file >> val) return val;
        return 0;
    }

    bool IioReader::FindDevices()
    {
        std::string base = "/sys/bus/iio/devices";
        if (!fs::exists(base)) return false;

        for (const auto& entry : fs::directory_iterator(base))
        {
            if (entry.is_directory())
            {
                std::string devPath = entry.path().string();
                std::string namePath = devPath + "/name";
                if (FileExists(namePath))
                {
                    std::ifstream nameFile(namePath);
                    std::string name;
                    nameFile >> name;
                    
                    if (name == "accel_3d" && !hasAccel)
                    {
                        accelPath = devPath;
                        hasAccel = true;
                        accelRawPaths[0] = accelPath + "/in_accel_x_raw";
                        accelRawPaths[1] = accelPath + "/in_accel_y_raw";
                        accelRawPaths[2] = accelPath + "/in_accel_z_raw";
                        
                        float scale = ReadFloat(accelPath + "/in_accel_scale");
                        accelScale[0] = accelScale[1] = accelScale[2] = scale;
                        log::Log("IioReader: Found accelerometer at " + accelPath + " with scale " + std::to_string(scale));
                    }
                    else if (name == "gyro_3d" && !hasGyro)
                    {
                        gyroPath = devPath;
                        hasGyro = true;
                        gyroRawPaths[0] = gyroPath + "/in_anglvel_x_raw";
                        gyroRawPaths[1] = gyroPath + "/in_anglvel_y_raw";
                        gyroRawPaths[2] = gyroPath + "/in_anglvel_z_raw";

                        float scale = ReadFloat(gyroPath + "/in_anglvel_scale");
                        gyroScale[0] = gyroScale[1] = gyroScale[2] = scale;
                        log::Log("IioReader: Found gyroscope at " + gyroPath + " with scale " + std::to_string(scale));
                    }
                    else if (name == "bmi323-imu" || name == "lsm6dsx" || name == "bmi160") // Legion Go IMUs
                    {
                        if (!hasAccel && FileExists(devPath + "/in_accel_x_raw"))
                        {
                            accelPath = devPath;
                            hasAccel = true;
                            accelRawPaths[0] = accelPath + "/in_accel_x_raw";
                            accelRawPaths[1] = accelPath + "/in_accel_y_raw";
                            accelRawPaths[2] = accelPath + "/in_accel_z_raw";
                            float scale = ReadFloat(accelPath + "/in_accel_scale");
                            accelScale[0] = accelScale[1] = accelScale[2] = scale;
                            log::Log("IioReader: Found IMU-accel at " + accelPath + " with scale " + std::to_string(scale));
                        }
                        if (!hasGyro && FileExists(devPath + "/in_anglvel_x_raw"))
                        {
                            gyroPath = devPath;
                            hasGyro = true;
                            gyroRawPaths[0] = gyroPath + "/in_anglvel_x_raw";
                            gyroRawPaths[1] = gyroPath + "/in_anglvel_y_raw";
                            gyroRawPaths[2] = gyroPath + "/in_anglvel_z_raw";
                            float scale = ReadFloat(gyroPath + "/in_anglvel_scale");
                            gyroScale[0] = gyroScale[1] = gyroScale[2] = scale;
                            log::Log("IioReader: Found IMU-gyro at " + gyroPath + " with scale " + std::to_string(scale));
                        }
                    }
                }
            }
        }
        return hasAccel || hasGyro;
    }

    void IioReader::Execute()
    {
        uint32_t increment = 0;
        sdgyrodsu::SdHidFrame* sdFrame = reinterpret_cast<sdgyrodsu::SdHidFrame*>(frame->data());

        // Constantes para la conversión
        // Steam Deck espera:
        // Accel: 1G = 0x4000 (16384)
        // Gyro: 1 deg/s = 16
        const float deckAcc1G = 16384.0f;
        const float deckGyro1dps = 16.0f;
        const float rad2deg = 180.0f / M_PI;

        while (ShouldContinue())
        {
            auto startTime = std::chrono::steady_clock::now();

            sdFrame->Header = 0x01000940; // Fake header
            sdFrame->Increment = increment++;

            if (hasAccel)
            {
                // IIO units are m/s^2. 
                // raw * scale = m/s^2
                // We want: value * deckAcc1G / 9.80665 = deck units
                float scale = accelScale[0] * deckAcc1G / 9.80665f;
                // Legion Go S mounting (based on iio-dsu-bridge): 
                // x: [1, 0, 0], y: [0, -1, 0], z: [0, 0, -1]
                // sdgyrodsu expects:
                // AxisRightToLeft (X)
                // AxisFrontToBack (Y)
                // AxisTopToBottom (Z)
                sdFrame->AccelAxisRightToLeft = (int16_t)(ReadInt(accelRawPaths[0]) * scale);
                sdFrame->AccelAxisFrontToBack = (int16_t)(-ReadInt(accelRawPaths[1]) * scale);
                sdFrame->AccelAxisTopToBottom = (int16_t)(-ReadInt(accelRawPaths[2]) * scale);
            }

            if (hasGyro)
            {
                // IIO units are rad/s.
                // raw * scale = rad/s
                // We want: value * rad2deg * deckGyro1dps = deck units
                float scale = gyroScale[0] * rad2deg * deckGyro1dps;
                // Legion Go S mounting (based on iio-dsu-bridge): 
                // x: [1, 0, 0], y: [0, -1, 0], z: [0, 0, -1]
                sdFrame->GyroAxisRightToLeft = (int16_t)(ReadInt(gyroRawPaths[0]) * scale);
                sdFrame->GyroAxisFrontToBack = (int16_t)(-ReadInt(gyroRawPaths[1]) * scale);
                sdFrame->GyroAxisTopToBottom = (int16_t)(-ReadInt(gyroRawPaths[2]) * scale);
            }

            {
                auto lock = frameServe.GetServeLock();
                // frame is already updated via sdFrame pointer
            }

            auto endTime = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
            if (elapsed < scanTimeUs)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(scanTimeUs - elapsed));
            }
        }
    }

    void IioReader::FlushPipes()
    {
        frameServe.Flush();
    }
}
