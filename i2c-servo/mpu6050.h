#pragma once

#include <stdint.h>
#include <mutex>
#include <vector>

#include "i2cdev.h"
#include "imu.h"
#include "status.h"

/*
 * IMU Interface
 */


class Mpu6050 : public IImu {
    protected:
        Mpu6050(I2CDev *dev);

    public:
        ~Mpu6050() final;

    public:
        void getAccel(float &x, float &y, float &z) final;
        void getGyro(float &x, float &y, float &z) final;

    public:
        friend Status<IImu*> NewMpu6050(I2CDev *i2c);
};


Status<IImu*> NewMpu6050(I2CDev *i2c);
Status<IImu*> NewMpu6050(const char *i2cDev, uint16_t i2cAddr);
