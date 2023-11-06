#include "hiddev/hiddevreader.h"
#include "log/log.h"
#include <fcntl.h>
#include <sys/select.h>

using namespace kmicki::log;

namespace kmicki::hiddev
{
    // Definition - ReadData
    HidDevReader::ReadData::ReadData(int const& _frameLen)
    : startMarker(0),
      Data(new std::vector<char>(_frameLen),
           new std::vector<char>(_frameLen), 
           new std::vector<char>(_frameLen)),
      Unsynced()
    { }

    HidDevReader::ReadData::~ReadData()
    {
        TryStopThenKill();
    }

    void HidDevReader::ReadData::FlushPipes()
    { }

    void HidDevReader::ReadData::SetStartMarker(std::vector<char> const& marker)
    {
        startMarker = marker;
    }
}