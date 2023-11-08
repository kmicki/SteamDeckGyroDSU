#include "hiddev/hiddevreader.h"
#include "log/log.h"
#include <fcntl.h>
#include <sys/select.h>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    static const int cFileScanTimeToTimeout = 2;

    // Definition - ReadDataFile
    HidDevReader::ReadDataFile::ReadDataFile(std::string const& _inputFilePath, int const& _frameLen, int const& _scanTimeUs)
    : inputFile(_inputFilePath,cFileScanTimeToTimeout*_scanTimeUs,false), ReadData(_frameLen*HidDevReader::cInputRecordLen)
    { }

    void HidDevReader::ReadDataFile::ReconnectInput()
    {
        DisconnectInput();
        Log("HidDevReader::ReadDataFile: Opening hiddev file.",LogLevelDebug);
        inputFile.Open();
    }

    void HidDevReader::ReadDataFile::DisconnectInput()
    {
        if(inputFile.IsOpen())
        {
            Log("HidDevReader::ReadDataFile: Closing hiddev file.",LogLevelDebug);
            inputFile.Close();
        }
    }

    uint32_t const& ExtractFirst4Bytes(std::vector<char> const& data)
    {
        return *(reinterpret_cast<uint32_t const*>(data.data()));
    }
 
    void HidDevReader::ReadDataFile::Execute()
    {
        static const int cReportMissedTicksPeriod = 250;
        int missedTicks = 0;
        int nonMissedTicks = 0;

        ReconnectInput();
        if(!inputFile.IsOpen())
        {
            { LogF() << "Error: " << errno; }
            throw std::runtime_error("HidDevReader::ReadDataFile: Problem opening hiddev file. Are priviliges granted?");
        }
        auto const& data = Data.GetPointerToFill();

        Log("HidDevReader::ReadDataFile: Started.",LogLevelDebug);

        while(ShouldContinue())
        {
            //tick.WaitForSignal();

            if(!ShouldContinue())
                break;

            auto readCnt = inputFile.Read(*data);

            if(readCnt == 0)
            {
                Log("HidDevReader::ReadDataFile: Waiting for data timed out.",LogLevelTrace);
                continue;
            }

            if(!CheckData(data,readCnt))
                continue;

            HandleMissedTicks("HidDevReader::ReadData","HID frames",Data.WasReceived(),missedTicks,cReportMissedTicksPeriod,nonMissedTicks);

            Data.SendData();
        }

        DisconnectInput();
        Log("HidDevReader::ReadDataFile: Stopped.",LogLevelDebug);
    }

    bool HidDevReader::ReadDataFile::CheckData(std::unique_ptr<std::vector<char>> const& data, ssize_t readCnt)
    {
        static const uint32_t cFirst4Bytes = 0xFFFF0002;
        static const uint32_t cFirst4BytesAlternative = 0xFFFF0001;

        bool inputFail = readCnt < data->size();
        bool startMarkerFail = false;

        if(!inputFail)
        {
            startMarkerFail = ExtractFirst4Bytes(*data) != cFirst4Bytes;
            if(startMarkerFail && startMarker.size() > 0 && ExtractFirst4Bytes(*data) == cFirst4BytesAlternative)
            {
                startMarkerFail = false;
                // Check special start marker
                for(int i = cByteposInput, j=0;j<startMarker.size();++j,i+=cInputRecordLen)
                    if(startMarker[j] != (*data)[i])
                    {
                        startMarkerFail = true;
                        break;
                    }
            }
        }

        if(inputFail || startMarkerFail)
        {
            if(inputFail)
                Log("HidDevReader::ReadDataFile: Reading from hiddev file failed.",LogLevelDebug);
            else
                Log("HidDevReader::ReadDataFile: Reading from hiddev file started in the middle of the HID frame.",LogLevelDebug);
            // Failed to read a frame
            // or start in the middle of the input frame
            ReconnectInput();
            Unsynced.SendSignal();
            return false;
        }
        return true;
    }
}