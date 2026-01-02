#pragma once

#include <stdint.h>
#include <mutex>
#include <vector>

#include "i2cdev.h"
#include "pwm.h"
#include "pwm_group.h"
#include "status.h"

/*
 * PCA9685 Controller
 *
 * This library drive a PCA9686 via I2C to control 16 PWM Channels.  All
 * channels share a fequency in the range of about 50Hz to 1500Hz, with
 * the default approximately 200Hz.  This provides about a 5ms wide wave
 * compatable with most servos.
 * 
 * Each channel's duty cycle can be specified either as a floating point
 * pulse width in milliseconds, or as a percentage duty cycle.  
 * 
 * NOTE: For accuracy, pca_adjust can be called to adjust the built-in
 *    clock to real or system time.   The adjustment factor dfaults to 
 *    0.965 which is average skew for the device.
 * 
 * To Use:
 *    1- Initialize an I2CDev_t with the bus and address of the PCA.
 *    2- Initialize the PCA_t with the I2CDev_t.
 *    3- Adjust the pulse of the channels by calling:
 *          pca_set_chan_duty(0.0 to 1.0) or
 *          pca_set_chan_pulse(0.0 to <pulse width in ms>)
 */


#define PCA9685_CHANNELS 16

class Pca9685Pwm;
class Pca9685Group;

class Pca9685Pwm : public PwmBase {
private:
    Pca9685Group *group;

private:
    Pca9685Pwm(Pca9685Group *group, uint32_t channel);

public:
    virtual Status<float> SetDuty(float duty) final;
    virtual Status<float> SetWidthMs(float ms) final;

    friend class Pca9685Group;
};

class Pca9685Group : public PwmGroupBase {
    private:
        std::shared_ptr<I2CDev> i2c;
        std::mutex mtx;
        std::vector<std::shared_ptr<Pca9685Pwm>> pwms;
        float offs[PCA9685_CHANNELS];
        float widthMs;

        Pca9685Group(std::shared_ptr<I2CDev> i2c);
        Error Init();
        Status<float> SetChannelDutyLocked_(uint32_t channel, float duty);
        Status<float> SetChannelDuty(uint32_t channel, float duty);
        Status<float> SetChannelWidthMs(uint32_t channel, float ms);

    public:
        virtual Status<float> SetHertz(float target) final;    
        virtual StatusPwmRef GetChannel(uint32_t cnt) final;
        friend class Pca9685Pwm;
        friend StatusShared<IPwmGroup> NewPca9685(std::shared_ptr<I2CDev> i2c);
        friend StatusShared<IPwmGroup> NewPca9685(const char *i2cDev, uint16_t i2cAddr);
};


StatusShared<IPwmGroup> NewPca9685(std::shared_ptr<I2CDev> i2c);
StatusShared<IPwmGroup> NewPca9685(const char *i2cDev, uint16_t i2cAddr);


