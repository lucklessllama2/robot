#include "pwm_mock.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <mutex>

#include "i2cdev.h"
#include "pwm.h"
#include "pwm_group.h"
#include "status.h"

#define PCA_CLOCK     25000000.0   // 25Mhz
#define PCA_MAX_FLOAT 4095.5
#define PCA_MAX_INT   ((uint16_t) 4095)

PwmMockPwm::PwmMockPwm(PwmMockGroup *group, uint32_t channel) : PwmBase(channel) {
    this->group = group;
}

Status<float> PwmMockPwm::SetDuty(float duty) {
    return this->group->SetChannelDuty(this->channel, duty);
}

Status<float> PwmMockPwm::SetWidthMs(float widthMs) {
    return this->group->SetChannelWidthMs(this->channel, widthMs);
}

PwmMockGroup::PwmMockGroup() : PwmGroupBase(PWMMOCK_CHANNELS) {
    this->clkAdjust = 0.96;
    this->epsilonMs = 0.005;  // Ignore width changes < 0.005ms

    // Create shared pointes for all the children.
    for (int a=0; a < PWMMOCK_CHANNELS; a++) {
        auto ptr = new PwmMockPwm(this, a);
        this->pwms.emplace_back(std::shared_ptr<PwmMockPwm>(ptr));
        this->offs[a] = 0x8000;  // Use an illegal value to ensure we set it.
    }
}

Error PwmMockGroup::Init() {
    Status<float> res = this->SetHertz(200.0);
    if (!res) {
        printf("Error of: %d\n", (int) res.Err());
        return res.Err();
    }

    for (int a=0; a < PWMMOCK_CHANNELS; a++) {
        SetChannelWidthMs(a, 1.5);
    }
    return Error::Okay;
}

Status<float> PwmMockGroup::SetHertz(float hertz) {
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

    uint8_t pre = (uint8_t) scale;
    this->hertz = (PCA_CLOCK / (float) (4096 * (pre + 1)));
    this->widthMs = 1000.0 / this->hertz;
    float actualHz = this->hertz / this->clkAdjust;
    float actualMs = this->widthMs / this->clkAdjust;
    printf("Hz=%f/%f, WidthMs=%f/%f, \n", this->hertz, actualHz, this->widthMs, actualMs);
    return Status<float>(actualHz);
}

Status<float> PwmMockGroup::SetChannelDutyLocked_(uint32_t chan, float percent) {
    if (chan >= PWMMOCK_CHANNELS) {
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
    return (off / PCA_MAX_INT);
}

Status<float>  PwmMockGroup::SetChannelDuty(uint32_t chan, float percent) {
    std::lock_guard<std::mutex> lock(this->mtx);
    return this->SetChannelDutyLocked_(chan, percent);
}

Status<float>  PwmMockGroup::SetChannelWidthMs(uint32_t chan, float widthMs) {
    std::lock_guard<std::mutex> lock(this->mtx);
    float percent = widthMs / this->widthMs;
    auto res = this->SetChannelDutyLocked_(chan, percent);
    if (!res) {
        return res;
    }
    return Status<float>(res.Value() * this->widthMs);
}

StatusShared<IPwm> PwmMockGroup::GetChannel(uint32_t chan) {
    if (chan >= PWMMOCK_CHANNELS) {
        return Error::BadRange;
    }
    return StatusShared<IPwm>(this->pwms[chan]);
}

StatusShared<IPwmGroup> NewPwmMock() {
    PwmMockGroup *grp = new PwmMockGroup();
    Error err = grp->Init();
    if (err != Error::Okay) {
        delete grp;
        return StatusShared<IPwmGroup>(err);
    }
    return StatusShared<IPwmGroup>(grp);
}

