#include "sdgyrodsu/presenter.h"
#include "sdgyrodsu/cemuhookadapter.h"

#include <ncurses.h>

using  namespace kmicki::cemuhook::protocol;

namespace kmicki::sdgyrodsu
{

    void Presenter::Initialize()
    {
        initscr();
        refresh();
    }

    void Presenter::Present(SdHidFrame const& frame)
    {
        static int maxSpan = 0;
        static uint32_t lastInc;
        static MotionData md;
        auto incSpan = frame.Increment-lastInc; 

        static float lastAccelPitch = 0;
        static float lastAccelRoll = 0;
        static float lastAccelYaw = 0;

        if(lastInc && incSpan > maxSpan)
            maxSpan = incSpan;

        lastInc = frame.Increment;

        CemuhookAdapter::SetMotionData(frame,md,lastAccelPitch,lastAccelRoll,lastAccelYaw);

        int k=0;
        move(++k,0); printw("INC  : %10d         ",frame.Increment);
        move(++k,0); printw("SPAN : %10d         ",incSpan);
        move(++k,0); printw("MAX  : %10d         ",maxSpan);
        ++k;
        move(++k,0); printw("A_P  : %10d         ",frame.AccelAxisPitch);
        move(++k,0); printw("A_Y  : %10d         ",frame.AccelAxisYaw);
        move(++k,0); printw("A_R  : %10d         ",frame.AccelAxisRoll);
        ++k;
        move(++k,0); printw("G_P  : %10d         ",frame.GyroAxisPitch);
        move(++k,0); printw("G_Y  : %10d         ",frame.GyroAxisYaw);
        move(++k,0); printw("G_R  : %10d         ",frame.GyroAxisRoll);
        ++k;
        move(++k,0); printw("U1   : %10d         ",frame.Unknown1);
        move(++k,0); printw("U2   : %10d         ",frame.Unknown2);
        move(++k,0); printw("U3   : %10d         ",frame.Unknown3);
        move(++k,0); printw("U4   : %10d         ",frame.Unknown4);        
        ++k;
        move(++k,0); printw("A_X : %1.3f          ",md.accX);
        move(++k,0); printw("A_Y : %1.3f          ",md.accY);
        move(++k,0); printw("A_Z:  %1.3f          ",md.accZ);
        ++k;
        move(++k,0); printw("G_P : %3.1f          ",md.pitch);
        move(++k,0); printw("G_Y : %3.1f          ",md.yaw);
        move(++k,0); printw("G_R : %3.1f          ",md.roll);
        ++k;
        move(++k,0); printw("Press any key to finish.");

        refresh();
    }

    void Presenter::Finish()
    {
        endwin();
    }
}