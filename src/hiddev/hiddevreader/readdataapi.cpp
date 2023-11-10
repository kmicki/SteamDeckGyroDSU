#include "hiddev/hiddevreader.h"
#include "hiddev/hidapidev.h"
#include "log/log.h"
#include <hidapi/hidapi.h>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    static const int cApiScanTimeToTimeout = 2;

    // Definition - ReadDataApi
    HidDevReader::ReadDataApi::ReadDataApi(uint16_t const& _vId, uint16_t const& _pId, const int& _interfaceNumber, int const& _frameLen, int const& _scanTimeUs)
    : vId(_vId), pId(_pId), ReadData(_frameLen), timeout(cApiScanTimeToTimeout*_scanTimeUs/1000),interfaceNumber(_interfaceNumber),noGyro(nullptr)
    { }

    void HidDevReader::ReadDataApi::SetNoGyro(SignalOut &_noGyro)
    {
        noGyro = &_noGyro;
    }
 
    void HidDevReader::ReadDataApi::Execute()
    {
        HidApiDev dev(vId,pId,interfaceNumber,timeout);
        
        Log("HidDevReader::ReadDataApi: Opening HID device.",LogLevelDebug);
        if(!dev.Open())
            throw std::runtime_error("HidDevReader::ReadDataApi: Problem opening HID device.");

        auto const& data = Data.GetPointerToFill();

        Log("HidDevReader::ReadDataApi: Started.",LogLevelDebug);

        while(ShouldContinue())
        {
            if(!ShouldContinue())
                break;

            if(noGyro && noGyro->TrySignal())
            {
                Log("HidDevReader::ReadDataApi: Try reenabling gyro.",LogLevelTrace);
                if(dev.EnableGyro())
                    Log("HidDevReader::ReadDataApi: Gyro reenabled.",LogLevelDebug);
                else
                    Log("HidDevReader::ReadDataApi: Gyro reenaling failed.");
                continue;
            }

            auto readCnt = dev.Read(*data);

            if(readCnt < data->size())
            {
                { LogF(LogLevelTrace) << "HidDevReader::ReadDataApi: Not enough bytes read: " << readCnt << "."; }
                continue;
            }

            if(readCnt == 0)
            {
                Log("HidDevReader::ReadDataApi: Waiting for data timed out.",LogLevelTrace);
                continue;
            }

            Data.SendData();
        }
    
        Log("HidDevReader::ReadDataApi: Closing HID device.",LogLevelDebug);
        dev.Close();
        
        Log("HidDevReader::ReadDataApi: Stopped.",LogLevelDebug);
    }
}