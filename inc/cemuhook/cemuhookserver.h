#ifndef _KMICKI_CEMUHOOK_CEMUHOOKSERVER_H_
#define _KMICKI_CEMUHOOK_CEMUHOOKSERVER_H_

#include "sdgyrodsu/cemuhookadapter.h"
#include "cemuhookprotocol.h"
#include "config.h"
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

        Server(sdgyrodsu::CemuhookAdapter & _motionSource, Config & _config);

        ~Server();

        private:

        struct Client
        {
            sockaddr_in address;
            uint32_t id;
            int sendTimeout;

            bool operator==(sockaddr_in const& other);
            bool operator!=(sockaddr_in const& other);
        };

        std::mutex mainMutex;
        std::mutex stopSendMutex;
        std::mutex socketSendMutex;
        std::shared_mutex clientsMutex;

        bool stop;
        bool stopSending;

        int socketFd;

        sdgyrodsu::CemuhookAdapter & motionSource;
        std::unique_ptr<std::thread> serverThread;

        void serverTask();
        void sendTask();
        void Start();

        VersionData versionAnswer;
        InfoAnswer infoDeckAnswer;
        InfoAnswer infoNoneAnswer;
        DataEvent dataAnswer;

        bool checkTimeout;

        void PrepareAnswerConstants();

        std::pair<uint16_t , void const*> PrepareVersionAnswer(uint32_t const& id);
        std::pair<uint16_t , void const*> PrepareInfoAnswer(uint32_t const& id, uint8_t const& slot);
        std::pair<uint16_t , void const*> PrepareDataAnswer(uint32_t const& d, uint32_t const& packet);
        std::pair<uint16_t , void const*> PrepareDataAnswerWithoutCrc(uint32_t const& d, uint32_t const& packet);
        void ModifyDataAnswerId(uint32_t const& id);
        void CalcCrcDataAnswer();

        std::vector<Client> clients;

        void CheckClientTimeout(std::unique_ptr<std::thread> & sendThread, bool increment);

        Config & config;
    };
}

#endif

