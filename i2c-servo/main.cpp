#include <stdio.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include "pca9685.h"
#include "servo.h"

#define NUM_TEST_SERVOS 4
const char *names[NUM_TEST_SERVOS] = {
    "SRV1",
    "SRV2",
    "SRV3",
    "SRV4",
};

Servo *servos[NUM_TEST_SERVOS];

void physics_loop(void) {
    auto previous = std::chrono::high_resolution_clock::now();
    while(1) {
        usleep(2000);
        auto current = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(current - previous);
        float fdelta = delta / 1000.0f;
        for (int a=0; a < NUM_TEST_SERVOS; a++) {
            servos[a]->ApplyPhysics(fdelta);
        }
        previous = current;
    }
}

int main(int argc, char *args[]) {
    (void) argc;
    (void) args;
    
    auto pca = NewPca9685("/dev/i2c-1", 0x40);
    if (!pca) {
        printf("Failed to create pca\n");
    }

    for (int a=0; a < NUM_TEST_SERVOS; a++) {
        Servo *servo = new Servo(names[a], SC_EX2940MGX);
        PwmRef pwm = pca->GetChannel(a);
        servo->SetPwm(pwm);
        servo->SetPosition(90.0);
        servos[a] = servo;
    }

    std::thread physics(physics_loop);
    uint32_t snum = 0;
    while(1) {
        Servo *servo = servos[snum];
        printf("%s: ", )
    };


    physics.join();
    return 0;
}