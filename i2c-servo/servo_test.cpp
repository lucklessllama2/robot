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
    
    printf("\n\nTesting SetPercentAbs\n");
    servo->SetPercentAbs(0.0f);
    printf("\tSet PercentAbs to MINIMUM, hit enter to confirm.");
    getchar();

    servo->SetPercentAbs(0.5f);
    printf("\tSet PercentAbs to middle, hit enter to confirm.");
    getchar();

    servo->SetPercentAbs(1.0f);
    printf("\tSet PercentAbs to MAXIMUM, hit enter to confirm.");
    getchar();

    printf("\n\nTesting SetDegreeAdj (no CAP)\n");
    servo->SetCapped(false);
    printf("Set DegreeAdj to -10, got %6.2f\n",  servo->SetDegreeAdj(-10.0f));
    getchar();
   
    printf("Set DegreeAdj to 90, got %6.2f\n",  servo->SetDegreeAdj(90.0f));
    getchar();

    printf("Set DegreeAdj to 200, got %6.2f\n",  servo->SetDegreeAdj(+200.0f));
    getchar();

    printf("\n\nTesting SetDegreeAdj (with CAP)\n");
    servo->SetCapped(true);
    printf("Setting angle to -10, got %6.2f\n",  servo->SetDegreeAdj(-10.0f));
    getchar();
   
    printf("Setting angle to 90,  got %6.2f\n",  servo->SetDegreeAdj(90.0f));
    getchar();

    printf("Setting angle to 200,  got %6.2f\n",  servo->SetDegreeAdj(+200.0f));
    getchar();

    printf("\n\nTesting SetTarget (no CAP, min V, max A)\n");
    servo->SetCapped(true);
    servo->SetVelocity(0.0);
    servo->SetAccel(100.0);
    servo->SetUpdating(true);
    printf("Setting angle to -10, got %6.2f\n",  servo->SetRotation(-10.0f));
    getchar();
   
    printf("Setting angle to 90,  got %6.2f\n",  servo->SetRotation(90.0f));
    getchar();

    printf("Setting angle to 200,  got %6.2f\n",  servo->SetRotation(+200.0f));
    getchar();

}