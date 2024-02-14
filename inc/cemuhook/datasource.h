#ifndef _KMICKI_CEMUHOOK_DATASOURCE_H_
#define _KMICKI_CEMUHOOK_DATASOURCE_H_

#include "cemuhookprotocol.h"

namespace kmicki::cemuhook
{
    // Data source for Cemuhook DSU Server
    class DataSource {
        public:
        virtual void Start() = 0;
        virtual void Stop() = 0;
        virtual bool IsControllerConnected() = 0;
        
        // Modifies controller data in place.
        // Returns number of frames to be replicated in next calls (in case of missing frames).
        virtual int const& SetDataNewFrame(cemuhook::protocol::MotionData &motion) = 0;
    };
}

#endif