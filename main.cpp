#include "includes.h"
#include "hiddevreader.h"
#include "sdhidframe.h"

using namespace kmicki::sdgyrodsu;

    #define FRAME_LEN 64
    #define FRAMECNT_PER_FILE 5000
    #define SCAN_PERIOD_MS 2

    typedef HidDevReader::frame_t frame_t;

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

    void drawData(frame_t frame,int num)
    {
        static int maxSpan = 0;
        static uint32_t lastInc;
        uint32_t newInc = *(reinterpret_cast<uint32_t *>(frame.data()+4));

        if(lastInc && newInc-lastInc > maxSpan)
            maxSpan = newInc-lastInc;

        move(0,0);
        printw("%5d %d %d             ",num,newInc-lastInc,maxSpan);
        move(2,0);
        int k = 2;
        for (int i = 0; i < FRAME_LEN; ++i) {
            printw("%.2x ", (unsigned char) frame[i]);
            if(i % 16 == 15)
                move(++k,0);
        }

        SdHidFrame* sd = reinterpret_cast<SdHidFrame*>(frame.data());

        ++k;
        move(++k,0); printw("U1: %d         ",sd->Unknown1);
        move(++k,0); printw("U2: %d         ",sd->Unknown2);
        move(++k,0); printw("U3: %d         ",sd->Unknown3);
        move(++k,0); printw("U4: %d         ",sd->Unknown4);
        ++k;
        move(++k,0); printw("A_RL: %d         ",sd->AccelAxisRightToLeft);
        move(++k,0); printw("A_TB: %d         ",sd->AccelAxisTopToBottom);
        move(++k,0); printw("A_FB: %d         ",sd->AccelAxisFrontToBack);
        ++k;
        move(++k,0); printw("G_RL: %d         ",sd->GyroAxisRightToLeft);
        move(++k,0); printw("G_TB: %d         ",sd->GyroAxisTopToBottom);
        move(++k,0); printw("G_FB: %d         ",sd->GyroAxisFrontToBack);

        refresh();

        lastInc = newInc;
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
    
    HidDevReader reader(hidno,FRAME_LEN,SCAN_PERIOD_MS);


    initscr();
    refresh();

    int part = 0;

    while(true) {
        std::ostringstream strStr;
        strStr << "dump" << std::setfill('0') << std::setw(3) << part << ".hex";

        std::ofstream output( strStr.str(), std::ios::binary );

        int i = 0;

        while(i < FRAMECNT_PER_FILE)
        {
            auto& frame = reader.GetNewFrame();
            output.write(frame.data(),frame.size());
            drawData(frame,i);
            reader.UnlockFrame();
            ++i;
        }
        output.close();
        ++part;
    }
    return 0;
}