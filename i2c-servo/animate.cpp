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
#define LIMIT_EPSILON 0.01f

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
const char *modeName[3] = { "MID", "FWD", "BAK" };

int main(int argc, char *args[]) {
    (void) argc;
    (void) args;

    ServosInit();

    char prompt[128];
    char backsp[128];
    memset(backsp, '\b', sizeof(backsp));
    backsp[127] = 0;
    prompt[127] = 0;

    for (int a=0; a < NUM_SERVOS; a++) {
        Servo *servo = ServosGet(0, a);
        servo->SetCapped(false);
        servo->SetVelocity(0.0);
        servo->SetAccel(100.0);
        servo->SetUpdating(true);
    }

    float angles[3][3];
    for (int a=0; a < 3; a++) {
        for (int b=0; b<3; b++) {
            angles[b][a] = 90.0;
        }
    }

    State *state = new State(3);
    Servo *srvShl = ServosGet(0, 0);
    Servo *srvUpr = ServosGet(0, 4);
    Servo *srvLwr = ServosGet(0, 8);

    bool first = true;

    while(1) {
        int key = state->GetKey();

        if (first != true && key == -1) {
            usleep(100000);
            continue;
        }
        first = false;

        float shl, upr, lwr;
        shl = angles[state->mode][0];
        upr = angles[state->mode][1];
        lwr = angles[state->mode][2];

        float dTrg;
        switch(state->chan) {
            case 0:
                dTrg = shl;
                break;
            case 1:
                dTrg = upr;
                break;
            case 2:
                dTrg = lwr;
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
        char out = (key > 0) ? key : ' ';
        size_t len = 0;
        switch(state->chan) {
            case 0:
                shl = dTrg;
                angles[state->mode][state->chan] = dTrg;
                len = snprintf(prompt, sizeof(prompt), "%s : %s [%6.3f]  %6.3f  %6.3f  %c",
                    modeName[state->mode], srvShl->GetName().c_str(), shl, upr, lwr, out);
                break;
            case 1:
                upr = dTrg;
                angles[state->mode][state->chan] = dTrg;
                len = snprintf(prompt, sizeof(prompt), "%s : %s % 6.3f [%6.3f]  %6.3f  %c",
                    modeName[state->mode], srvUpr->GetName().c_str(), shl, upr, lwr, out);
                break;
            case 2:
                lwr = dTrg;
                angles[state->mode][state->chan] = dTrg;
                len = snprintf(prompt, sizeof(prompt), "%s : %s  %6.3f  %6.3f [%6.3f] %c",
                    modeName[state->mode], srvLwr->GetName().c_str(), shl, upr, lwr, out);
                break;
        }
        srvShl->SetRotation(shl);
        srvUpr->SetRotation(upr);
        srvLwr->SetRotation(lwr);
        
        if (len != 0) {
            backsp[len+2] = 0;
            printf("%s%s", backsp, prompt);
            fflush(stdout);
        
            // Restore Backspace.
            backsp[len+2] = '\b';
        }
        usleep(100000);
    };

    return 0;
}