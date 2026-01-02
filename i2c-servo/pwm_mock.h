#pragma once

#include <stdint.h>
#include <mutex>
#include <vector>

#include "i2cdev.h"
#include "pwm.h"
#include "pwm_group.h"
#include "status.h"

/*
 * PwmMock Controller
 *
 */


#define PWMMOCK_CHANNELS 16

class PwmMockPwm;
class PwmMockGroup;

class PwmMockPwm : public PwmBase {
private:
    PwmMockGroup *group;

private:
    PwmMockPwm(PwmMockGroup *group, uint32_t channel);

public:
    virtual Status<float> SetDuty(float duty) final;
    virtual Status<float> SetWidthMs(float ms) final;

    friend class PwmMockGroup;
};

class PwmMockGroup : public PwmGroupBase {
    private:
        std::mutex mtx;
        std::vector<std::shared_ptr<PwmMockPwm>> pwms;
        float offs[PWMMOCK_CHANNELS];
        float widthMs;

        PwmMockGroup();
        Error Init();
        Status<float> SetChannelDutyLocked_(uint32_t channel, float duty);
        Status<float> SetChannelDuty(uint32_t channel, float duty);
        Status<float> SetChannelWidthMs(uint32_t channel, float ms);

    public:
        virtual Status<float> SetHertz(float target) final;    
        virtual StatusPwmRef GetChannel(uint32_t cnt) final;
        friend class PwmMockPwm;
        friend StatusShared<IPwmGroup> NewPwmMock();
};

StatusShared<IPwmGroup> NewPwmMock();


