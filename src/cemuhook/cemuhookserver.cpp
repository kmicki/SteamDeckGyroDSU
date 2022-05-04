#include "cemuhookserver.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdexcept>
#include <unistd.h>

using namespace kmicki::sdgyrodsu;

#define PORT 26760
#define BUFLEN 100

namespace kmicki::cemuhook
{
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
        char buf[BUFLEN];
        sockaddr_in sockInClient;
        socklen_t sockInLen = sizeof(sockInClient);

        while(!stop)
        {
            auto recvLen = recvfrom(socketFd,buf,BUFLEN,0,(sockaddr*) &sockInClient, &sockInLen);
            if(recvLen > 0)
            {
                sendto(socketFd,buf,recvLen,0,(sockaddr*) &sockInClient, sockInLen);
            }
        }
    }
}