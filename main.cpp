#include "includes.h"

namespace kmicki::sdgyrodsu {

    #define INPUT_FRAME_LEN 512
    #define FRAME_LEN 64
    #define FRAMECNT_PER_FILE 5000
    #define TIMEOUT_MAX_REPEATS 500
    #define BYTEPOS_INPUT 4
    #define INPUT_RECORD_LEN 8
    #define INPUT_FRAME_TIMEOUT_MS 5
    #define FREQ_DIVIDER 1
    #define SCAN_PERIOD_MS 4

    typedef std::array<char,FRAME_LEN> frameType;
    typedef std::array<char,INPUT_FRAME_LEN> inputFrameType;

    // Execute shell command, provide stdout and return code returned by the command
    int exec(std::string cmd, std::string &stdout) {
        std::array<char, 128> buffer;
        stdout.clear();
        FILE * pipe;
        try {
            pipe = popen(cmd.c_str(), "r");
            if (!pipe) {
                throw std::runtime_error("popen() failed!");
            }
            while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                stdout += buffer.data();
            }
            return pclose(pipe);
        }
        catch(...) 
        {
            if(pipe)
                pclose(pipe);
            return -1;
        }
    }

    // find which X among /dev/usb/hiddevX fits Steam Deck Controls
    int findHidDevNo() 
    {
        std::string output;
        int bus,dev,bus2,dev2;
        int testlen;
        // Steam Deck controls: usb device VID: 28de, PID: 1205
        if(exec("lsusb | grep '28de:1205' | sed -e \"s/.*Bus \\([0-9]\\+\\) Device \\([0-9]\\+\\).*/\\1 \\2/g\"",output) || (testlen = output.length()) != 8)
            return -1;

        std::istringstream str(output);
        str >> bus >> dev;
        if (bus == 0 || dev == 0)
            return -1;

        for (int i = 0; i < 10; i++)    
        {
            std::ostringstream ostr;
            ostr << "udevadm info --query=all /dev/usb/hiddev" << i << " | grep 'P:' | sed -e \"s/.*usb\\([0-9]\\+\\).*\\.\\([0-9]\\+\\).*/\\1 \\2/g\"";
            std::string cmd = ostr.str();
            if(exec(cmd,output) || output.length() > 8 ||  output.length() < 4)
                continue;
            std::istringstream str(output);
            str >> bus2 >> dev2;
            if (bus == bus2 || dev == dev2)
                return i;
        }
        return -1;
    }

    // Extract HID frame from data read from /dev/usb/hiddevX
    void processData(inputFrameType const& bufIn, std::ofstream *os, frameType &bufOut) 
    {
        // Each byte is encapsulated in an 8-byte long record
        for (int i = 0, j = BYTEPOS_INPUT; i < FRAME_LEN; ++i,j+=INPUT_RECORD_LEN) {
            bufOut[i] = bufIn[j];
        }

        os->write(bufOut.data(),FRAME_LEN);
    }

    void drawData(frameType frame,int num)
    {
        static uint32_t lastInc;
        uint32_t newInc = *(reinterpret_cast<uint32_t *>(frame.data()+4));

        move(0,0);
        printw("%5d %d             ",num,newInc-lastInc);
        move(2,0);
        int k = 2;
        for (int i = 0; i < FRAME_LEN; ++i) {
            printw("%.2x ", (unsigned char) frame[i]);
            if(i % 16 == 15)
                move(++k,0);
        }
        refresh();

        lastInc = newInc;
    }
}

using namespace kmicki::sdgyrodsu;

static std::exception_ptr teptr = nullptr;

int main()
{
    int hidno = findHidDevNo();
    if(hidno < 0) 
    {
        std::cerr << "Device not found." << std::endl;
        return 0;
    }

    std::cout << "Found hiddev" << hidno << std::endl << "Press any key";
    std::cin.get();
    std::ostringstream hiddevStr;
    hiddevStr << "/dev/usb/hiddev" << hidno;

    int part = 0;
    std::ifstream *input = new std::ifstream( hiddevStr.str(), std::ios::binary );
    frameType frame;

    if(input->fail()) 
    {
        std::cerr << "Failed to open " << hiddevStr.str() << ". Are priviliges granted?" << std::endl;
        return 1;
    }


    initscr();
    refresh();

    while(true) {
        //std::cout << "Part " << std::setfill('0') << std::setw(3) << part << std::endl;
        std::ostringstream strStr;
        strStr << "dump" << std::setfill('0') << std::setw(3) << part << ".hex";

        std::ofstream output( strStr.str(), std::ios::binary );

        inputFrameType buf;
        int i = 0;
        int fail = 0;
        std::unique_ptr<std::future<void>> process = nullptr;
        bool checkedStart = false;

        std::unique_ptr<std::future<void>> scan = nullptr;

        while(i < FRAMECNT_PER_FILE)
        {
            
            // Skip some frames
            for(int j = 0; j< FREQ_DIVIDER -1; ++j)
            {
                std::thread skipThread(static_cast<std::istream& (std::istream::*)(std::streamsize)>(&std::ifstream::ignore),input,INPUT_FRAME_LEN);
                auto handle = skipThread.native_handle();   // this handle is needed to cancel the reading thread in case of block,
                                                            // it has to be retrieved and stored before calling join on the thread.
                auto timeoutSkip=std::async(std::launch::async, &std::thread::join, &skipThread);
                if (timeoutSkip.wait_for(std::chrono::milliseconds(INPUT_FRAME_TIMEOUT_MS)) == std::future_status::timeout) 
                {
                    // If reading didn't finish in defined time, cancel reading thread and repeat reading in next iteration
                    // Steam deck controls send data 500 times a second so 2 ms should be enough 
                    //std::cout << "FREQFAIL" << std::endl;
                    ++fail;
                    --j;
                    pthread_cancel(handle);
                    if(fail > TIMEOUT_MAX_REPEATS) 
                    {
                        std::cerr << "Steam Deck controls stopped generating reports. Make sure Steam is running." << std::endl;
                        return 2;
                    }
                }
            }
            
            if(scan.get() != nullptr)
                scan.get()->wait();
            scan.reset(new std::future<void>(std::async(std::launch::async,std::this_thread::sleep_for<int64_t,std::milli>,std::chrono::milliseconds(SCAN_PERIOD_MS))));

            fail = 0;    

            // Reading from hiddev is done in a separate thread.
            // Then future is created that waits for the reading to finish (by executing join).
            // It is necessary because if 2 consecutive reads are done too quickly (no new frame ready)
            // then ifstream::read is blocked indefinitely.
            std::thread t1(&std::ifstream::read,input,buf.data(),INPUT_FRAME_LEN);
            auto handle = t1.native_handle();   // this handle is needed to cancel the reading thread in case of block,
                                                // it has to be retrieved and stored before calling join on the thread.
            auto timeoutFuture=std::async(std::launch::async, &std::thread::join, &t1);
            if (timeoutFuture.wait_for(std::chrono::milliseconds(INPUT_FRAME_TIMEOUT_MS)) == std::future_status::timeout) {
                // If reading didn't finish in defined time, cancel reading thread and repeat reading in next iteration
                // Steam deck controls send data 500 times a second so 2 ms should be enough 
                //std::cout << "FAIL" << std::endl;
                ++fail;
                pthread_cancel(handle);
                if(fail > TIMEOUT_MAX_REPEATS) 
                {
                    std::cerr << "Steam Deck controls stopped generating reports. Make sure Steam is running." << std::endl;
                    return 2;
                }
            }
            else if(!checkedStart && (buf[0] != (char)0x02 || buf[1] != (char)0x00 || buf[2] != (char)0xFF || buf[3] != (char)0xFF)) 
            {
                //std::cout << "Start in the middle of the frame. Try again." << std::endl;
                input->close();
                delete input;
                input = new std::ifstream( hiddevStr.str(), std::ios::binary );
                //checkedStart = true;
            }
            else {
                // reading of the frame succeded
                fail = 0;
                // Start processing data in a separate thread unless previous processing has not finished
                if(process.get() == nullptr || process.get()->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {
                    drawData(frame,i);
                    process.reset(new std::future<void>(std::async(std::launch::async, processData, std::ref(buf), &output, std::ref(frame))));
                    //std::cout << i << std::endl;
                    ++i;
                }
                else
                    ;//std::cout << "SKIP" << std::endl;
            }
        }
        if(process != nullptr)
            process->wait(); // finished processing last frame in the part
        output.close();
        ++part;
    }
    input->close();
    delete input;
    return 0;
}