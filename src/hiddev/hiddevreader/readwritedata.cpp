#include "hiddev/hiddevreader.h"
#include "hiddev/hidapidev.h"
#include "log/log.h"
#include <hidapi/hidapi.h>
#include <string>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    namespace rwd {
        static const std::string cLogPrefix = "HidDevReader::ReadWriteData: ";
        #include "log/locallog.h"
    }

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
        if(IsStarted())
            throw std::runtime_error(rwd::cLogPrefix + "Cannot set write data - operation is already started.");
        writeData = &_writeData;
    }
 
    void HidDevReader::ReadWriteData::Execute()
    {
        HidApiDev dev(vId,pId,interfaceNumber,timeout);
        
        rwd::Log("Opening HID device.",LogLevelDebug);
        if(!dev.Open())
            throw std::runtime_error(rwd::cLogPrefix + "Problem opening HID device.");

        auto const& _readData = ReadData.GetPointerToFill();

        rwd::Log("Started.",LogLevelDebug);

        while(ShouldContinue())
        {
            if(!ShouldContinue())
                break;

            if(writeData && writeData->TryData(true))
            {
                rwd::Log("Try writing data to device.",LogLevelTrace);
                if(dev.Write(writeData->GetData()))
                    rwd::Log("Date written to device.",LogLevelDebug);
                else
                    rwd::Log("Data writing to device failed.");
                continue;
            }

            auto readCnt = dev.Read(*_readData);

            if(readCnt < _readData->size())
            {
                { rwd::LogF(LogLevelTrace) << "Not enough bytes read: " << readCnt << "."; }
                continue;
            }

            if(readCnt == 0)
            {
                rwd::Log("Waiting for data timed out.",LogLevelTrace);
                continue;
            }

            ReadData.SendData();
        }
    
        rwd::Log("Closing HID device.",LogLevelDebug);
        dev.Close();
        
        rwd::Log("Stopped.",LogLevelDebug);
    }
}