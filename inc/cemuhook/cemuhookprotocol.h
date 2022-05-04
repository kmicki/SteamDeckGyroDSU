#ifndef _KMICKI_CEMUHOOK_CEMUHOOKPROTOCOL_H_
#define _KMICKI_CEMUHOOK_CEMUHOOKPROTOCOL_H_

#include <cstdint>

namespace kmicki::cemuhook::protocol
{

    struct Header
    {
        char magic[4]; // DSUS - server, DSUC - client
        uint16_t version; // 1001
        uint16_t length; // without header
        uint32_t crc32; // whole packet with this field = 0
        uint32_t id; // of packet source, constant among one run
        uint32_t eventType; // no part of the header where length is involved
        // event types:
        //  0x100000 - protocol version information
        //  0x100001 - information about connected controllers
        //  0x100002 - actual controllers data
    };

    struct VersionData
    {
        uint16_t version;
    };

    struct SharedResponse
    {
        uint8_t slot;
        uint8_t slotState; // 0 - not connected, 1 - reserved, 2 - connected
        uint8_t deviceModel; // 0 - not applicable, 1 - no or partial gyro, 2 - full gyro, 3 - do not use
        uint8_t connection; // 0 - not applicable, 1 - USB, 2 - bluetooth
        uint32_t mac1; // unused - 0
        uint16_t max2; // unused - 0
        uint8_t battery; // unused - 0
    };

    struct InfoRequest
    {
        int32_t portCnt; // amount of ports to report
    };

    struct InfoAnswer
    {
        Header header;
        SharedResponse response;
        char end; // end of packet = 0x0
    };

    struct SubscribeRequest
    {
        uint8_t mask; // 1 slot-based, 2 - mac-base, 3 - both, 0 - all controllers
        uint8_t slot; // slot to subscribe
        uint32_t mac1; // unused
        uint16_t mac2; // unused
    };

    struct MotionData
    {
        uint64_t timestamp;
        float accX;
        float accY;
        float accZ;
        float pitch;
        float yaw;
        float roll;
    };

    struct DataEvent
    {
        Header header;
        SharedResponse response;
        uint8_t connected; // 1 - connected
        uint32_t packetNumber;
        uint8_t buttons1;
        uint8_t buttons2;
        uint8_t homeButton;
        uint8_t touchButton;
        uint8_t lsX;
        uint8_t lsY;
        uint8_t rsX;
        uint8_t rsY;
        uint8_t adLeft;
        uint8_t adDown;
        uint8_t adRight;
        uint8_t adUp;
        uint8_t aY;
        uint8_t aB;
        uint8_t aA;
        uint8_t aX;
        uint8_t aR1;
        uint8_t aL1;
        uint8_t aR2;
        uint8_t aL2;
        uint32_t touch[3];
        MotionData motion;
    };

}

#endif