#pragma once

#include <atomic>

class State {
    public:
        State(int modes);
        ~State();

    public:
        bool ChangeServo() const { return newChan || newGroup; }
        bool ChangeMode() const  { return newMode; }

    public:
        int  GetKey();

    public:
        int group;
        int chan;
        int mode;

    protected:
        int  modeCount;
        bool newChan;
        bool newMode;
        bool newGroup;
};
