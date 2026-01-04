#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "config.h"
#include "pca9685.h"
#include "pwm_mock.h"
#include "state.h"
#include "servo.h"


volatile bool debug = 0;

std::atomic<bool> quit = false;

void servo_loop(Servo **servos) {
    auto previous = std::chrono::high_resolution_clock::now();
    while(!quit.load()) {
        usleep(2000);

        auto current = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(current - previous);
        float fdelta = (float) delta.count() / 1000.0f;
        for (int a=0; a < NUM_SERVOS; a++) {
           // servos[a]->Update(fdelta);
        }
        previous = current;
    }
}

const char *bankName[2] = { "FWD", "BAK"};
const char *modeName[2] = { "<LO", "HI>"};

int main(int argc, char *args[]) {
    (void) argc;
    (void) args;

    ServosInit();

    char prompt[128];
    char backsp[128];
    memset(backsp, '\b', sizeof(backsp));
    backsp[127] = 0;
    prompt[127] = 0;


    State *state = new State(2);
    float dTrg;

    bool first = true;    
    while(1) {
        Servo *servo = ServosGet(state->group, state->chan);
        int key = state->GetKey();

        float dMin, dMax, dTrg;
        servo->GetDegCap(dMin, dMax);            
        switch(state->mode) {
            case 0:
                dTrg = dMin;
                break;
            case 1:
                dTrg = dMax;
                break;
        }

        switch(key) {
            case '{':
                dTrg -= 2.5;
                break;
            case '}':
                dTrg += 2.5;
                break;
            case '[':
                dTrg -= 0.1;
                break;
            case ']':
                dTrg += 0.1;
                break;
        } 
        
        dTrg = std::min(dTrg, 180.0f);
        dTrg = std::max(dTrg, 0.0f);
        switch(state->mode) {
            case 0:
                dMin = dTrg;
                break;
            case 1:
                dMax = dTrg;
                break;
        }
        servo->SetDegCap(dMin, dMax);
        servo->SetRotation(dTrg);
        char dirty = servo->IsDirty() ? '*' : ' ';

        size_t len = snprintf(prompt, sizeof(prompt), "%c %s %s=%6.3f",
            dirty, servo->GetName().c_str(), modeName[state->mode], dTrg);
        
        backsp[len+2] = 0;
        printf("%s%s", backsp, prompt);
        fflush(stdout);
        
        // Restore Backspace.
        backsp[len+2] = '\b';
        usleep(100000);
    };

    return 0;
}