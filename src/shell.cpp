#include "shell.h"
#include <array>
#include <stdexcept>

namespace kmicki::shell
{
    int ExecuteCommand(std::string cmd, std::string &stdout)
    {
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
}