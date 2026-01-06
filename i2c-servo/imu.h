#pragma once

#include <stdint.h>
#include <mutex>
#include <vector>

#include "i2cdev.h"
#include "pwm.h"
#include "pwm_group.h"
#include "status.h"

/*
 * IMU Interface
 */


class IImu {
    public:
        virtual ~IImu() = 0;

    public:
        void getAccel(float &x, float &y, float &z) = 0;
        void getGyro(float &x, float &y, float &z) = 0;
};

