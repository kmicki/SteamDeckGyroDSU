#ifndef _KMICKI_HIDDEV_IIOIMU_H_
#define _KMICKI_HIDDEV_IIOIMU_H_

#include <array>
#include <string>
#include <cstdint>

namespace kmicki::hiddev
{
    // Maps sensor axes (x, y, z) onto one output axis, with a sign.
    struct IioAxisMap
    {
        int source; // 0 - x, 1 - y, 2 - z
        int sign;   // 1 or -1
    };

    // Mapping of IIO sensor axes onto Steam Deck HID frame axes.
    // Order of the outputs is the order of fields in the Steam Deck frame:
    //   accel[0] - AccelAxisRightToLeft, accel[1] - AccelAxisTopToBottom, accel[2] - AccelAxisFrontToBack
    //   gyro[0]  - GyroAxisRightToLeft,  gyro[1]  - GyroAxisTopToBottom,  gyro[2]  - GyroAxisFrontToBack
    struct IioImuConfig
    {
        std::string devicePath; // e.g. /sys/bus/iio/devices/iio:device0
        std::array<IioAxisMap,3> accel;
        std::array<IioAxisMap,3> gyro;
    };

    // Find an IIO device that provides both accelerometer and gyroscope raw channels.
    // Returns path in /sys/bus/iio/devices or empty string if none found.
    // name - name of the device that is found, if found.
    std::string FindIioImu(std::string &name);

    // Parse axis mapping string like "x,-y,z". Returns false if the string is invalid.
    bool ParseIioAxisMap(std::string const& str, std::array<IioAxisMap,3> &map);

    // Reads accelerometer and gyroscope raw values from IIO device sysfs files.
    class IioImu
    {
        public:
        IioImu() = delete;
        IioImu(std::string const& devicePath);
        ~IioImu();

        bool Open();
        void Close();
        bool IsOpen() const;

        // Read accelerometer axis (0..2). Value in m/s^2.
        bool ReadAccel(int axis, double &value);
        // Read gyroscope axis (0..2). Value in rad/s.
        bool ReadGyro(int axis, double &value);

        private:
        bool ReadRaw(int fd, long &value);
        bool ReadDouble(std::string const& file, double &value, double defaultValue);

        std::string path;
        std::array<int,3> accelFd;
        std::array<int,3> gyroFd;
        std::array<double,3> accelScale;
        std::array<double,3> gyroScale;
        std::array<double,3> accelOffset;
        std::array<double,3> gyroOffset;
    };
}

#endif
