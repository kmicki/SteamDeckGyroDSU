#ifndef _KMICKI_SDGYRODSU_SDHIDFRAME_H_
#define _KMICKI_SDGYRODSU_SDHIDFRAME_H_

#include <cstdint>
#include "hiddev/hiddevreader.h"

namespace kmicki::sdgyrodsu
{
    typedef kmicki::hiddev::HidDevReader::frame_t frame_t;

    struct SdHidFrame 
    {
        uint32_t Header;
        uint32_t Increment;

        // Buttons 1:
        // 	.0 - R2 full pull
        // 	.1 - L2 full pull
        // 	.2 - R1
        // 	.3 - L1
        // 	.4 - Y
        // 	.5 - B
        // 	.6 - X
        // 	.7 - A
        //	.12 - Select
        //  .13 - STEAM
        //  .14 - Start
        //  .15 - L5
        //	.16 - R5
        //	.17 - L trackpad click
        //	.18 - R trackpad click
        //	.19 - L trackpad touch
        //	.20 - R trackpad touch
        //	.22 - L3
        //	.26 - R3
        uint32_t Buttons1; 

        // Buttons 2:
        //  .9 - L4
        //  .10 - R4
        //  .14 - L3 touch
        //  .15 - R3 touch
        //	.18 - (...)
        uint32_t Buttons2;

        int16_t LeftTrackpadX;
        int16_t LeftTrackpadY;
        int16_t RightTrackpadX;
        int16_t RightTrackpadY;

        int16_t AccelAxisRightToLeft; // axis from right side to left side of the SD
        int16_t AccelAxisTopToBottom; // axis from top side to bottom side of the SD
        int16_t AccelAxisFrontToBack; // axis from front side to back side of the SD

        // Gyro + -> movement rotating in unscrewing direction (left) around axis
        int16_t GyroAxisRightToLeft; // axis from right side to left side of the SD
        int16_t GyroAxisTopToBottom; // axis from top side to bottom side of the SD
        int16_t GyroAxisFrontToBack; // axis from front side to back side of the SD

        int16_t Unknown1;
        int16_t Unknown2;
        int16_t Unknown3;
        int16_t Unknown4;

        int16_t L2Analog;
        int16_t R2Analog;
        int16_t LeftStickX;
        int16_t LeftStickY;
        int16_t RightStickX;
        int16_t RightStickY;

        int16_t LeftTrackpadPushForce;
        int16_t RightTrackpadPushForce;
        int16_t LeftStickTouchCoverage;
        int16_t RightStickTouchCoverage;

        
    };

    SdHidFrame const& GetSdFrame(frame_t const& frame);

}

#endif