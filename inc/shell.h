#ifndef _KMICKI_SHELL_SHELL_H_
#define _KMICKI_SHELL_SHELL_H_

#include <string>

namespace kmicki::shell
{
    // Execute shell command, provide stdout and return code returned by the command
    int ExecuteCommand(std::string cmd, std::string &stdout);
}

#endif
