#include "hiddev/hiddevreader.h"
#include "hiddev/hidapidev.h"
#include "log/log.h"
#include <hidapi/hidapi.h>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    static const int cApiScanTimeToTimeout = 2;

    

    // Definition - ReadWriteData
    HidDevReader::ReadWriteData::ReadWriteData(uint16_t const& _vId, uint16_t const& _pId, const int& _interfaceNumber, int const& _frameLen, int const& _scanTimeUs)
    : vId(_vId), pId(_pId), interfaceNumber(_interfaceNumber),
      timeout(cApiScanTimeToTimeout*_scanTimeUs/1000), writeData(nullptr),
      ReadData(new frame_t(_frameLen),
               new frame_t(_frameLen), 
               new frame_t(_frameLen)),
      Unsynced()
    { }

    HidDevReader::ReadWriteData::~ReadWriteData()
    {
        TryStopThenKill();
    }

    void HidDevReader::ReadWriteData::FlushPipes()
    { }

    void HidDevReader::ReadWriteData::SetWriteData(PipeOut<frame_t> &_writeData)
    {
        writeData = &_writeData;
    }
 
    void HidDevReader::ReadWriteData::Execute()
    {
        HidApiDev dev(vId,pId,interfaceNumber,timeout);
        
        Log("HidDevReader::ReadDataApi: Opening HID device.",LogLevelDebug);
        if(!dev.Open())
            throw std::runtime_error("HidDevReader::ReadDataApi: Problem opening HID device.");

        auto const& _readData = ReadData.GetPointerToFill();

        Log("HidDevReader::ReadDataApi: Started.",LogLevelDebug);

        while(ShouldContinue())
        {
            if(!ShouldContinue())
                break;

            if(writeData && writeData->TryData())
            {
                Log("HidDevReader::ReadDataApi: Try writing data to device.",LogLevelTrace);
                if(dev.Write(writeData->GetData()))
                    Log("HidDevReader::ReadDataApi: Gyro reenabled.",LogLevelDebug);
                else
                    Log("HidDevReader::ReadDataApi: Gyro reenaling failed.");
                continue;
            }

            auto readCnt = dev.Read(*_readData);

            if(readCnt < _readData->size())
            {
                { LogF(LogLevelTrace) << "HidDevReader::ReadDataApi: Not enough bytes read: " << readCnt << "."; }
                continue;
            }

            if(readCnt == 0)
            {
                Log("HidDevReader::ReadDataApi: Waiting for data timed out.",LogLevelTrace);
                continue;
            }

            ReadData.SendData();
        }
    
        Log("HidDevReader::ReadDataApi: Closing HID device.",LogLevelDebug);
        dev.Close();
        
        Log("HidDevReader::ReadDataApi: Stopped.",LogLevelDebug);
    }
}