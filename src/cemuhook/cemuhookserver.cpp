#include "cemuhookserver.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <stdexcept>
#include <unistd.h>
#include <iostream>

using namespace kmicki::sdgyrodsu;

#define PORT 26760
#define BUFLEN 100
#define SCANTIME 0
#define DECKSLOT 0
#define SENDTIMEOUT_X 3

#define VERSION_TYPE 0x100000
#define INFO_TYPE 0x100001
#define DATA_TYPE 0x100002

namespace kmicki::cemuhook
{

    uint32_t crc32(const unsigned char *s,size_t n) {
        uint32_t crc=0xFFFFFFFF;
        
        int k;

        while (n--) {
            crc ^= *s++;
            for (k = 0; k < 8; k++)
                crc = crc & 1 ? (crc >> 1) ^ 0xedb88320  : crc >> 1;
        }
        return ~crc;
    }

    Server::Server(CemuhookAdapter & _motionSource)
        : motionSource(_motionSource), stop(false), serverThread(), stopSending(false),
          mainMutex(), stopSendMutex(), socketSendMutex()
    {
        PrepareAnswerConstants();
        Start();
    }
    
    Server::~Server()
    {
        if(serverThread.get() != nullptr)
        {
            {
                std::lock_guard lock(mainMutex);
                stop = true;
            }
            serverThread.get()->join();
        }
        if(socketFd > -1)
            close(socketFd);
    }

    void Server::Start() 
    {
        std::cout << "Cemuhook Server: Initializing." << std::endl;
        if(serverThread.get() != nullptr)
        {
            {
                std::lock_guard lock(mainMutex);
                stop = true;
            }
            serverThread.get()->join();
            serverThread.reset();
        }

        socketFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        timeval read_timeout;
        read_timeout.tv_sec = 2;
        read_timeout.tv_usec = 0;
        setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

        if(socketFd == -1)
            throw std::runtime_error("Socket could not be created.");
        
        sockaddr_in sockInServer;

        sockInServer = sockaddr_in();

        sockInServer.sin_family = AF_INET;
        sockInServer.sin_port = htons(PORT);
        sockInServer.sin_addr.s_addr = INADDR_ANY;

        if(bind(socketFd, (sockaddr*)&sockInServer, sizeof(sockInServer)) < 0)
            throw std::runtime_error("Bind failed.");

        stop = false;
        serverThread.reset(new std::thread(&Server::serverTask,this));
        std::cout << "Cemuhook Server: Initialized." << std::endl;
    }

    void Server::PrepareAnswerConstants()
    {
        Header outHeader;
        outHeader.magic[0] = 'D';
        outHeader.magic[1] = 'S';
        outHeader.magic[2] = 'U';
        outHeader.magic[3] = 'S';
        outHeader.version = 1001;

        versionAnswer.header = outHeader;
        versionAnswer.header.length = sizeof(versionAnswer.version) + 4;
        versionAnswer.version = 1001;

        SharedResponse sresponse;
        sresponse.deviceModel = 2;
        sresponse.connection = 1;
        sresponse.mac1 = 0;
        sresponse.mac2 = 0;
        sresponse.battery = 0;
        sresponse.connected = 0;

        infoDeckAnswer.header = outHeader;
        infoDeckAnswer.header.eventType = INFO_TYPE;
        infoDeckAnswer.header.length = sizeof(infoDeckAnswer.response) + 4;
        infoDeckAnswer.response = sresponse;
        infoDeckAnswer.response.slot = DECKSLOT;

        infoNoneAnswer = infoDeckAnswer;
        infoNoneAnswer.response.deviceModel = 0;
        infoNoneAnswer.response.connection = 0;
        infoNoneAnswer.response.slotState = 0;

        dataAnswer.header = outHeader;
        dataAnswer.header.eventType = DATA_TYPE;
        dataAnswer.header.length = sizeof(dataAnswer) - 16;
        dataAnswer.response = sresponse;
        dataAnswer.response.slotState = 2;
        dataAnswer.response.connected = 1;
        dataAnswer.response.slot = DECKSLOT;
        
        char* dataAnswerPointer = reinterpret_cast<char*>(&dataAnswer.buttons1);
        auto len = sizeof(DataEvent) - sizeof(Header) - sizeof(SharedResponse) - sizeof(MotionData);
        for (int i = 0; i < len; i++)
        {
            // clear most data
            dataAnswerPointer[i] = 0;
        }
    }

    void Server::serverTask()
    {
        char buf[BUFLEN];
        sockaddr_in sockInClient,sendSockInClient;
        socklen_t sockInLen = sizeof(sockInClient);

        auto headerSize = (ssize_t)sizeof(Header);

        std::pair<uint16_t , void const*> outBuf;

        std::unique_ptr<std::thread> sendThread;

        int sendTimeout = 0;

        std::cout << "Cemuhook Server: Start listening for client." << std::endl;
        
        std::unique_lock mainLock(mainMutex);
        while(!stop)
        {
            mainLock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            auto recvLen = recvfrom(socketFd,buf,BUFLEN,0,(sockaddr*) &sockInClient, &sockInLen);
            if(recvLen >= headerSize)
            {
                sendTimeout = 0;
                Header & header = *reinterpret_cast<Header*>(buf);

                switch(header.eventType)
                {
                    case VERSION_TYPE:
                        outBuf = PrepareVersionAnswer(header.id);
                        {
                            std::lock_guard lock(socketSendMutex);
                            sendto(socketFd,outBuf.second,outBuf.first,0,(sockaddr*) &sockInClient, sockInLen);
                        }
                        break;
                    case INFO_TYPE:
                        {
                            InfoRequest & req = *reinterpret_cast<InfoRequest*>(buf+headerSize);
                            for (int i = 0; i < req.portCnt; i++)
                            {
                                outBuf = PrepareInfoAnswer(header.id, req.slots[i]);
                                {
                                    std::lock_guard lock(socketSendMutex);
                                    sendto(socketFd,outBuf.second,outBuf.first,0,(sockaddr*) &sockInClient, sockInLen);
                                }
                            }
                        }
                        break;
                    case DATA_TYPE:
                        if(sendThread.get() != nullptr 
                           && (sockInClient.sin_addr.s_addr != sendSockInClient.sin_addr.s_addr
                               || sockInClient.sin_port != sendSockInClient.sin_port))
                        {
                            {
                                std::lock_guard lock(stopSendMutex);
                                stopSending = true;
                            }
                            sendThread.get()->join();
                            //std::this_thread::sleep_for(std::chrono::seconds(1));
                            sendThread.reset();
                        }
                        if(sendThread.get() == nullptr)
                        {
                            std::cout << "Cemuhook Server: Client subscribed to data events." << std::endl;
                            stopSending = false;
                            sendThread.reset(new std::thread(&Server::sendTask,this,sockInClient, header.id));
                            sendSockInClient = sockInClient;
                        }
                        break;
                }
            }
            else 
            {
                ++sendTimeout;
                if(sendTimeout > SENDTIMEOUT_X)
                {       
                    if(sendThread.get() != nullptr)
                    {
                        std::cout << "Cemuhook Server: No packet from client for some time. Stop sending data." << std::endl;
                        {
                            std::lock_guard lock(stopSendMutex);
                            stopSending = true;
                        }
                        sendThread.get()->join();
                        //std::this_thread::sleep_for(std::chrono::seconds(1));
                        sendThread.reset();
                    }
                    sendTimeout = 0;
                }
                    
                    
            }
            mainLock.lock();
        }
        if(sendThread.get() != nullptr)
        {
            {
                std::lock_guard lock(stopSendMutex);
                stopSending = true;
            }
            sendThread.get()->join();
        }
    }

    void Server::sendTask(sockaddr_in sockInClient, uint32_t id)
    {
        std::cout << "Cemuhook Server: Initiaiting frame grab start." << std::endl;
        motionSource.StartFrameGrab();

        std::pair<uint16_t , void const*> outBuf;
        uint32_t packet = 0;

        std::cout << "Cemuhook Server: Start sending controller data." << std::endl;

        std::unique_lock mainLock(stopSendMutex);

        while(!stopSending)
        {
            mainLock.unlock();
            outBuf = PrepareDataAnswer(id,packet++);
            {
                std::lock_guard lock(socketSendMutex);
                sendto(socketFd,outBuf.second,outBuf.first,0,(sockaddr*) &sockInClient, sizeof(sockInClient));
            }
            mainLock.lock();
        }

        std::cout << "Cemuhook Server: Initiating frame grab stop." << std::endl;

        motionSource.StopFrameGrab();
        std::cout << "Cemuhook Server: Stop sending controller data." << std::endl;
    }


    std::pair<uint16_t , void const*> Server::PrepareVersionAnswer(uint32_t id)
    {
        static const uint16_t len = sizeof(versionAnswer);

        versionAnswer.header.id = id;
        versionAnswer.header.crc32 = 0;
        versionAnswer.header.crc32 = crc32(reinterpret_cast<unsigned char *>(&versionAnswer),len);
            return std::pair<uint16_t , void const*>(len, reinterpret_cast<void *>(&versionAnswer));
    }

    std::pair<uint16_t , void const*> Server::PrepareInfoAnswer(uint32_t id, uint8_t slot)
    {
        static const uint16_t len = sizeof(infoNoneAnswer);

        if(slot != DECKSLOT)
        {
            infoNoneAnswer.header.id = id;
            infoNoneAnswer.response.slot = slot;
            infoNoneAnswer.header.crc32 = 0;
            infoNoneAnswer.header.crc32 = crc32(reinterpret_cast<unsigned char *>(&infoNoneAnswer),len);
            return std::pair<uint16_t , void const*>(len, reinterpret_cast<void *>(&infoNoneAnswer));
        }
        
        infoDeckAnswer.header.id = id;
        infoDeckAnswer.response.slot = slot;
        infoDeckAnswer.response.slotState = motionSource.IsControllerConnected()?2:0;
        infoDeckAnswer.header.crc32 = 0;
        infoDeckAnswer.header.crc32 = crc32(reinterpret_cast<unsigned char *>(&infoDeckAnswer),len);
        return std::pair<uint16_t , void const*>(len, reinterpret_cast<void *>(&infoDeckAnswer));
    }

    std::pair<uint16_t , void const*> Server::PrepareDataAnswer(uint32_t id, uint32_t packet) 
    {
        static const uint16_t len = sizeof(dataAnswer);
        
        dataAnswer.header.id = id;
        dataAnswer.packetNumber = packet;
        dataAnswer.motion = motionSource.GetMotionDataNewFrame();
        
        dataAnswer.header.crc32 = 0;
        dataAnswer.header.crc32 = crc32(reinterpret_cast<unsigned char *>(&dataAnswer),len);
        return std::pair<uint16_t , void const*>(len, reinterpret_cast<void *>(&dataAnswer));

    }


}