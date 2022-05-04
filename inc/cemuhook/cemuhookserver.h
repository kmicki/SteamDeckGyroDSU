#ifndef _KMICKI_CEMUHOOK_CEMUHOOKSERVER_H_
#define _KMICKI_CEMUHOOK_CEMUHOOKSERVER_H_

#include "cemuhookadapter.h"

namespace kmicki::cemuhook
{
    class Server
    {
        public:
        Server() = delete;

        Server(sdgyrodsu::CemuhookAdapter motionSource);

        ~Server();

        private:


    }
}

#endif

