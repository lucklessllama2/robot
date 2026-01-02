#include "pca9685.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <mutex>

#include "i2cdev.h"
#include "pwm.h"
#include "pwm_group.h"
#include "status.h"

#define PCA_CLOCK     25000000.0   // 25Mhz
#define PCA_HERTZ     300
#define PCA_MAX_FLOAT 4095.5
#define PCA_MAX_INT   ((uint16_t) 4095)

enum {
    PCA_MODE1_REG = 0,          // Mode 1 Register offset
    PCA_MODE2_REG = 1,          // Mode 2 Register offset
    PCA_LED0_ON_REG = 6,        // First channel starting at On
    PCA_LED0_OFF_REG = 8,       // First channel starting at Off
    PCA_SCALE_REG = 254,        // Prescale register offset.
};

enum {
    PCA_MODE1_ALLCALL = 0x01,   // Respond to All Call Addr
    PCA_MODE1_SUB3    = 0x02,   // Respond to SUB3 Addr
    PCA_MODE1_SUB2    = 0x04,   // Respond to SUB2 Addr
    PCA_MODE1_SUB1    = 0x08,   // Respond to SUB1 Addr
    PCA_MODE1_SLEEP   = 0x10,   // 0=Runing, 1=Sleep (OSC off)
    PCA_MODE1_INCR    = 0x20,   // Enable auto-incr register
    PCA_MODE1_EXTCLK  = 0x40,   // 0=Internal, 1=External OSC.
    PCA_MODE1_RESTART = 0x80,   // Push 1 to restart from sleep.
};

enum {
    PCA_MODE2_OE_VAL  = 0x01,   // When set OE=1, out=0 or 1 
    PCA_MODE2_OE_HIZ  = 0x02,   // When OE=1, set high impedence.  
    PCA_MODE2_OUTDRV  = 0x04,   // Drive (0=Open Drain, 1=Push/Pull)
    PCA_MODE2_OCH     = 0x08,   // Change On (0=Stop, 1=ACK)
    PCA_MODE2_INVT    = 0x10,   // Invert output
};

enum {
    PCA_LED_ON_L  = 0,  // Offset of bits 7:0 for ON timing.
    PCA_LED_ON_H  = 1,  // Offset of bits 11:8 for ON timing.
    PCA_LED_OFF_L = 2,  // Offset of bits 7:0 for OFF timing.
    PCA_LED_OFF_H = 3,  // Offset of bits 11:8 for OFF timing.
    PCA_LED_WIDTH = 4,  // Number of 8b registers per channel.
};

Pca9685Pwm::Pca9685Pwm(Pca9685Group *group, uint32_t channel) : PwmBase(channel) {
    this->group = group;
}

Status<float> Pca9685Pwm::SetDuty(float duty) {
    return this->group->SetChannelDuty(this->channel, duty);
}

Status<float> Pca9685Pwm::SetWidthMs(float widthMs) {
    return this->group->SetChannelWidthMs(this->channel, widthMs);
}


Pca9685Group::Pca9685Group(std::shared_ptr<I2CDev> i2c) : PwmGroupBase(PCA9685_CHANNELS) {
    this->clkAdjust = 0.96;
    this->epsilonMs = 0.005;  // Ignore width changes < 0.005ms
    this->i2c = i2c;


    // Create shared pointes for all the children.
    for (int a=0; a < PCA9685_CHANNELS; a++) {
        auto ptr = new Pca9685Pwm(this, a);
        this->pwms.emplace_back(std::shared_ptr<Pca9685Pwm>(ptr));
        this->offs[a] = 0x8000;  // Use an illegal value to ensure we set it.
    }
}

Error Pca9685Group::Init() {
    // Set Auto Inc and Push Pull
    uint8_t cmd[] = { 
        PCA_MODE1_INCR | PCA_MODE1_SLEEP,  // Set auto-inc, internal clock, and sleep.
        PCA_MODE2_OUTDRV,   // Set standard push-pull mode.
    };

    Error err = this->i2c->WriteRegs(PCA_MODE1_REG, cmd, sizeof(cmd));
    if (err != Error::Okay) {
        return err;
    }

    Status<float> res = this->SetHertz(PCA_HERTZ);
    if (!res) {
        printf("Error of: %d\n", (int) res.Err());
        return res.Err();
    }

    printf("Setting channels.\n");
    for (int a=0; a < PCA9685_CHANNELS; a++) {
        SetChannelWidthMs(a, 1.5);
    }
    printf("Done channels.\n");
    return Error::Okay;
}

Status<float> Pca9685Group::SetHertz(float hertz) {
    float clock = PCA_CLOCK * this->clkAdjust;
    float prescale = clock / (4096.0 * hertz);
    int scale = (int) (prescale + 0.5) - 1;
    // Prescale contains what we need to device the clock by, to hit
    // the hertz we are looking for.  The divider works as follows:
    // 1 tick = (CLOCK / (250))
    if (scale < 3 || scale > 255) {
        printf("Failed, bad range.\n");
        return Error::BadRange;
    }

    // Go to sleep so we can change the clock.
    uint8_t cmd = PCA_MODE1_INCR | PCA_MODE1_SLEEP;
    auto err = this->i2c->WriteRegs(PCA_MODE1_REG, &cmd, 1);
    if (err != Error::Okay) {
        printf("failed to sleep...");
        return err;
    }
    usleep(10);

    uint8_t pre = (uint8_t) scale;
    err = this->i2c->WriteRegs(PCA_SCALE_REG, &pre, 1);
    if (err != Error::Okay) {
        printf("failed to write regs...");
        return err;
    }

    cmd = PCA_MODE1_INCR | PCA_MODE1_RESTART;
    err = this->i2c->WriteRegs(PCA_MODE1_REG, &cmd, 1);
    if (err != Error::Okay) {
        printf("failed to sleep...");
        return err;
    }    

    // Now that we reset the scale, let's calculate the ACTUAL 
    // Hertz and Width.
    this->hertz = (PCA_CLOCK / (float) (4096 * (pre + 1)));
    this->widthMs = 1000.0 / this->hertz;
    float actualHz = this->hertz / this->clkAdjust;
    float actualMs = this->widthMs / this->clkAdjust;
    printf("Hz=%f/%f, WidthMs=%f/%f, \n", this->hertz, actualHz, this->widthMs, actualMs);
    return Status<float>(actualHz);
}

Status<float> Pca9685Group::SetChannelDutyLocked_(uint32_t chan, float percent) {
    if (chan >= PCA9685_CHANNELS) {
        return Error::BadIndex;
    }
    if (percent < 0.0) {
        return Error::BadRange;
    }
    uint16_t off = (percent * PCA_MAX_FLOAT) * this->clkAdjust;
    off = std::min(PCA_MAX_INT, off);

    if (off == this->offs[chan]) {
        return off;
    }

    // Assume ON is at ZERO to reduce bandwidth.
    uint8_t cmd[5];
    cmd[0] = (chan * PCA_LED_WIDTH) + PCA_LED0_ON_REG;
    cmd[1] = 0;
    cmd[2] = 0;
    cmd[3] = off & 0xFF;
    cmd[4] = (off >> 8) & 0x0F;
    auto err = this->i2c->WriteBlock(cmd, sizeof(cmd));
    if (err == Error::Okay) {
        this->offs[chan] = off;
        return (off / PCA_MAX_INT);
    }
    return err;
}

Status<float>  Pca9685Group::SetChannelDuty(uint32_t chan, float percent) {
    std::lock_guard<std::mutex> lock(this->mtx);
    return this->SetChannelDutyLocked_(chan, percent);
}

Status<float>  Pca9685Group::SetChannelWidthMs(uint32_t chan, float widthMs) {
    std::lock_guard<std::mutex> lock(this->mtx);
    float percent = widthMs / this->widthMs;
    auto res = this->SetChannelDutyLocked_(chan, percent);
    if (!res) {
        return res;
    }
    return Status<float>(res.Value() * this->widthMs);
}

StatusShared<IPwm> Pca9685Group::GetChannel(uint32_t chan) {
    if (chan >= PCA9685_CHANNELS) {
        return Error::BadRange;
    }
    return StatusShared<IPwm>(this->pwms[chan]);
}

StatusShared<IPwmGroup> NewPca9685(std::shared_ptr<I2CDev> i2c) {
    Pca9685Group *grp = new Pca9685Group(i2c);
    Error err = grp->Init();
    if (err != Error::Okay) {
        delete grp;
        return StatusShared<IPwmGroup>(err);
    }
    return StatusShared<IPwmGroup>(grp);
}


StatusShared<IPwmGroup> NewPca9685(const char *i2cDev, uint16_t i2cAddr) {
    auto i2c = NewI2CDev(i2cDev, i2cAddr);
    if (!i2c) {
        return StatusShared<IPwmGroup>(i2c.Err());
    }

    return NewPca9685(i2c.Ptr());
}
