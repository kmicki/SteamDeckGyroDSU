#ifndef _KMICKI_CEMUHOOK_CEMUHOOKSERVER_H_
#define _KMICKI_CEMUHOOK_CEMUHOOKSERVER_H_

#include "cemuhookadapter.h"
#include "cemuhookprotocol.h"
#include <thread>
#include <netinet/in.h>
#include <mutex>

using namespace kmicki::cemuhook::protocol;

namespace kmicki::cemuhook
{
    class Server
    {
        public:
        Server() = delete;

        Server(sdgyrodsu::CemuhookAdapter & _motionSource);

        ~Server();

        private:

        std::mutex mainMutex;
        std::mutex stopSendMutex;
        std::mutex socketSendMutex;

        bool stop;
        bool stopSending;

        int socketFd;

        sdgyrodsu::CemuhookAdapter & motionSource;
        std::unique_ptr<std::thread> serverThread;

        void serverTask();
        void sendTask(sockaddr_in sockInClient, uint32_t id);
        void Start();

        VersionData versionAnswer;
        InfoAnswer infoDeckAnswer;
        InfoAnswer infoNoneAnswer;
        DataEvent dataAnswer;

        void PrepareAnswerConstants();

        std::pair<uint16_t , void const*> PrepareVersionAnswer(uint32_t id);
        std::pair<uint16_t , void const*> PrepareInfoAnswer(uint32_t id, uint8_t slot);
        std::pair<uint16_t , void const*> PrepareDataAnswer(uint32_t id, uint32_t packet);
    };
}

#endif

