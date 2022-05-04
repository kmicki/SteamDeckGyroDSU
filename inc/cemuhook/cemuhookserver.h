#ifndef _KMICKI_CEMUHOOK_CEMUHOOKSERVER_H_
#define _KMICKI_CEMUHOOK_CEMUHOOKSERVER_H_

#include "cemuhookadapter.h"
#include <thread>

namespace kmicki::cemuhook
{
    class Server
    {
        public:
        Server() = delete;

        Server(sdgyrodsu::CemuhookAdapter & _motionSource);

        ~Server();

        private:

        bool stop;
        int socketFd;

        sdgyrodsu::CemuhookAdapter & motionSource;
        std::unique_ptr<std::thread> serverThread;

        void serverTask();
        void Start();
    };
}

#endif

