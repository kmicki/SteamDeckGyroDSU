#include "presenter.h"

#include <ncurses.h>


namespace kmicki::sdgyrodsu
{
    void Presenter::Initialize()
    {
        initscr();
        refresh();
    }

    void Presenter::Present(frame_t frame)
    {
        static int maxSpan = 0;
        static uint32_t lastInc;
        uint32_t newInc = *(reinterpret_cast<uint32_t *>(frame.data()+4));

        if(lastInc && newInc-lastInc > maxSpan)
            maxSpan = newInc-lastInc;

        move(0,0);
        printw("%d %d             ",newInc-lastInc,maxSpan);
        move(2,0);
        int k = 2;
        for (int i = 0; i < frame.size(); ++i) {
            printw("%.2x ", (unsigned char) frame[i]);
            if(i % 16 == 15)
                move(++k,0);
        }

        SdHidFrame* sd = reinterpret_cast<SdHidFrame*>(frame.data());

        ++k;
        move(++k,0); printw("INC: %d         ",sd->Increment);

        ++k;
        move(++k,0); printw("U1: %d         ",sd->Unknown1);
        move(++k,0); printw("U2: %d         ",sd->Unknown2);
        move(++k,0); printw("U3: %d         ",sd->Unknown3);
        move(++k,0); printw("U4: %d         ",sd->Unknown4);
        ++k;
        move(++k,0); printw("A_RL: %d         ",sd->AccelAxisRightToLeft);
        move(++k,0); printw("A_TB: %d         ",sd->AccelAxisTopToBottom);
        move(++k,0); printw("A_FB: %d         ",sd->AccelAxisFrontToBack);
        ++k;
        move(++k,0); printw("G_RL: %d         ",sd->GyroAxisRightToLeft);
        move(++k,0); printw("G_TB: %d         ",sd->GyroAxisTopToBottom);
        move(++k,0); printw("G_FB: %d         ",sd->GyroAxisFrontToBack);

        ++k;
        move(++k,0); printw("Press any key to finish.");

        refresh();

        lastInc = newInc;
    }

    void Presenter::Finish()
    {
        endwin();
    }
}