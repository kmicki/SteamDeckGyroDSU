#include "hiddev/iioimu.h"
#include "log/log.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    static const std::string cIioDevicesPath = "/sys/bus/iio/devices";
    static const char cAxes[3] = { 'x', 'y', 'z' };

    static bool FileExists(std::string const& path)
    {
        std::error_code ec;
        return std::filesystem::exists(path,ec);
    }

    std::string FindIioImu(std::string &name)
    {
        std::error_code ec;
        if(!std::filesystem::is_directory(cIioDevicesPath,ec))
            return std::string();

        for(auto const& entry : std::filesystem::directory_iterator(cIioDevicesPath,ec))
        {
            auto path = entry.path().string();
            bool complete = true;
            for(char axis : cAxes)
            {
                if(!FileExists(path + "/in_accel_" + axis + "_raw") || !FileExists(path + "/in_anglvel_" + axis + "_raw"))
                {
                    complete = false;
                    break;
                }
            }
            if(!complete)
                continue;

            std::ifstream nameFile(path + "/name");
            name.clear();
            std::getline(nameFile,name);
            return path;
        }

        return std::string();
    }

    bool ParseIioAxisMap(std::string const& str, std::array<IioAxisMap,3> &map)
    {
        std::array<IioAxisMap,3> result;
        std::stringstream ss(str);
        std::string item;
        int i = 0;
        while(std::getline(ss,item,','))
        {
            if(i >= 3)
                return false;
            int sign = 1;
            if(!item.empty() && (item[0] == '-' || item[0] == '+'))
            {
                sign = (item[0] == '-') ? -1 : 1;
                item = item.substr(1);
            }
            if(item.size() != 1 || item[0] < 'x' || item[0] > 'z')
                return false;
            result[i] = { item[0]-'x', sign };
            ++i;
        }
        if(i != 3)
            return false;
        map = result;
        return true;
    }

    IioImu::IioImu(std::string const& devicePath)
    : path(devicePath)
    {
        accelFd.fill(-1);
        gyroFd.fill(-1);
        accelScale.fill(1.0);
        gyroScale.fill(1.0);
        accelOffset.fill(0.0);
        gyroOffset.fill(0.0);
    }

    IioImu::~IioImu()
    {
        Close();
    }

    bool IioImu::ReadDouble(std::string const& file, double &value, double defaultValue)
    {
        std::ifstream f(file);
        if(!f.is_open())
        {
            value = defaultValue;
            return false;
        }
        f >> value;
        if(f.fail())
        {
            value = defaultValue;
            return false;
        }
        return true;
    }

    bool IioImu::Open()
    {
        Close();

        for(int i = 0; i < 3; ++i)
        {
            std::string a = path + "/in_accel_" + cAxes[i];
            std::string g = path + "/in_anglvel_" + cAxes[i];

            // Scale/offset are per-axis or shared for all axes of the sensor type.
            if(!ReadDouble(a + "_scale",accelScale[i],1.0))
                ReadDouble(path + "/in_accel_scale",accelScale[i],1.0);
            if(!ReadDouble(g + "_scale",gyroScale[i],1.0))
                ReadDouble(path + "/in_anglvel_scale",gyroScale[i],1.0);
            if(!ReadDouble(a + "_offset",accelOffset[i],0.0))
                ReadDouble(path + "/in_accel_offset",accelOffset[i],0.0);
            if(!ReadDouble(g + "_offset",gyroOffset[i],0.0))
                ReadDouble(path + "/in_anglvel_offset",gyroOffset[i],0.0);

            accelFd[i] = open((a + "_raw").c_str(),O_RDONLY);
            gyroFd[i] = open((g + "_raw").c_str(),O_RDONLY);
            if(accelFd[i] < 0 || gyroFd[i] < 0)
            {
                Close();
                return false;
            }
        }

        { LogF(LogLevelDebug) << "IioImu: Opened " << path << ". Accel scale: " << accelScale[0] << " Gyro scale: " << gyroScale[0]; }

        return true;
    }

    void IioImu::Close()
    {
        for(auto* fds : { &accelFd, &gyroFd })
            for(auto &fd : *fds)
            {
                if(fd >= 0)
                    close(fd);
                fd = -1;
            }
    }

    bool IioImu::IsOpen() const
    {
        return accelFd[0] >= 0;
    }

    bool IioImu::ReadRaw(int fd, long &value)
    {
        char buf[32];
        auto cnt = pread(fd,buf,sizeof(buf)-1,0);
        if(cnt <= 0)
            return false;
        buf[cnt] = 0;
        char *end;
        value = strtol(buf,&end,10);
        return end != buf;
    }

    bool IioImu::ReadAccel(int axis, double &value)
    {
        long raw;
        if(!ReadRaw(accelFd[axis],raw))
            return false;
        value = ((double)raw + accelOffset[axis]) * accelScale[axis];
        return true;
    }

    bool IioImu::ReadGyro(int axis, double &value)
    {
        long raw;
        if(!ReadRaw(gyroFd[axis],raw))
            return false;
        value = ((double)raw + gyroOffset[axis]) * gyroScale[axis];
        return true;
    }
}
