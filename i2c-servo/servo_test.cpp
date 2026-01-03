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


int main(int argc, char *argv[]) {
    ServosInit();
    ServosDump();

    Servo *servo = ServosGet(0, 0);
    printf("\n\n\n!!!!!  Test will set servo beyond cap ranges. !!!!!\n");
    printf("Are you sure you want to continue?  Hit CTRL-C to abort.\n");

    servo->SetPercentAbs(0.0f);
    printf("Setting pulse to MINIMUM, hit enter to confirm.");
    getchar();

    servo->SetPercentAbs(0.5f);
    printf("Setting pulse to middle, hit enter to confirm.");
    getchar();

    servo->SetPercentAbs(1.0f);
    printf("Setting pulse to MAXIMUM, hit enter to confirm.");
    getchar();


    servo->SetCapped(false);
    printf("Setting angle to -10, got %6.2f\n",  servo->SetDegreeAdj(-10.0f));
    getchar();
   
    printf("Setting angle to 90,  got %6.2f\n",  servo->SetDegreeAdj(90.0f));
    getchar();

    printf("Setting angle to 200,  got %6.2f\n",  servo->SetDegreeAdj(+200.0f));
    getchar();

    printf("Imposing cap.\n");
    servo->SetCapped(true);
    printf("Setting angle to -10, got %6.2f\n",  servo->SetDegreeAdj(-10.0f));
    getchar();
   
    printf("Setting angle to 90,  got %6.2f\n",  servo->SetDegreeAdj(90.0f));
    getchar();

    printf("Setting angle to 200,  got %6.2f\n",  servo->SetDegreeAdj(+200.0f));
    getchar();
}