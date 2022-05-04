#include "cemuhookserver.h"
#include "cemuhookprotocol.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdexcept>
#include <unistd.h>

using namespace kmicki::sdgyrodsu;
using namespace kmicki::cemuhook::protocol;

#define PORT 26760
#define BUFLEN 100

namespace kmicki::cemuhook
{

    uint32_t crc32(const char *s,size_t n) {
        uint32_t crc=0xFFFFFFFF;
        
        for(size_t i=0;i<n;i++) {
            char ch=s[i];
            for(size_t j=0;j<8;j++) {
                uint32_t b=(ch^crc)&1;
                crc>>=1;
                if(b) crc=crc^0xEDB88320;
                ch>>=1;
            }
        }
        
        return ~crc;
    }

    Server::Server(CemuhookAdapter & _motionSource)
        : motionSource(_motionSource), stop(false), serverThread()
    {
        Start();
    }
    
    Server::~Server()
    {
        if(serverThread.get() != nullptr)
        {
            stop = true;
            serverThread.get()->join();
        }
        if(socketFd > -1)
            close(socketFd);
    }

    void Server::Start() 
    {
        if(serverThread.get() != nullptr)
        {
            stop = true;
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

        serverThread.reset(new std::thread(&Server::serverTask,this));
    }

    void Server::serverTask()
    {
        uint8_t deckSlot = 0;

        char buf[BUFLEN];
        sockaddr_in sockInClient;
        socklen_t sockInLen = sizeof(sockInClient);

        auto headerSize = sizeof(Header);

        Header outHeader;
        outHeader.magic[0] = 'D';
        outHeader.magic[1] = 'E';
        outHeader.magic[2] = 'C';
        outHeader.magic[3] = 'K';
        outHeader.version = 1001;

        VersionData version;
        version.version = 1001;

        SharedResponse sresponseDeck;
        sresponseDeck.deviceModel = 2;
        sresponseDeck.connection = 1;
        sresponseDeck.mac1 = 0;
        sresponseDeck.mac2 = 0;
        sresponseDeck.battery = 0;

        SharedResponse sresponseOther;
        sresponseOther = sresponseDeck;
        sresponseOther.slotState = 0;
        sresponseOther.deviceModel = 0;
        sresponseOther.connection = 0;
        sresponseOther.connected = 0;

        InfoAnswer infoDeck;
        infoDeck.response = sresponseDeck;

        auto versionLen = sizeof(VersionData)-16;

        while(!stop)
        {
            auto recvLen = recvfrom(socketFd,buf,BUFLEN,0,(sockaddr*) &sockInClient, &sockInLen);
            if(recvLen >= headerSize)
            {
                Header & header = *reinterpret_cast<Header*>(buf);
                outHeader.id = header.id;
                outHeader.eventType = header.eventType;
                
                switch(header.eventType)
                {
                    case 0x100000:
                        version.header = outHeader;
                        version.header.length = versionLen;
                        version.header.crc32 = 0;
                        version.header.crc32 = crc32(reinterpret_cast<char*>(&version),sizeof(version));
                        sendto(socketFd,&version,sizeof(version),0,(sockaddr*) &sockInClient, sockInLen);
                        break;
                    case 0x100001:
                        //sresponseDeck.slotState = motionSource.IsControllerConnected()?2:0;
                        break;
                }
            }
        }
    }
}