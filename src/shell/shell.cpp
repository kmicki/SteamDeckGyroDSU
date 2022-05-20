#include "shell.h"
#include <array>
#include <stdexcept>

namespace kmicki::shell
{
    // Execute shell command and provide output (stdout) and exit code
    // cmd: command to execute
    // stdout: string variable to store output to
    // return value: exit code of the command
    int ExecuteCommand(std::string cmd, std::string &stdout)
    {
        std::array<char, 128> buffer;
        stdout.clear();
        FILE * pipe;
        try {
            pipe = popen(cmd.c_str(), "r"); // execute command and connect to pipe for output
            if (!pipe) {
                throw std::runtime_error("popen() failed!");
            }

            // read output until no more available
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