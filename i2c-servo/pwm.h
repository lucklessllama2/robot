#pragma once

/*
 * PWM Interface
 *
 * This file defines the abstract interface for a single PWM Channel.  It
 * can be used to set the Duty Cycle (0.0-1.0) of the PWM, or to set the
 * Width (in milliseconds) of the pulse.
 *
 * Then frequency of the PWM is controlled by the owning PWM Group and 
 * can not be configured individually.  It's expected that individual
 * PWMs are constructed and provided by the owning group.
 */

#include <stdint.h>
#include "status.h"

class IPwm;
class IPwmGroup;

// The PWM Interface defining the only functions expected to be used
// externally.
class IPwm {
    public:
        virtual ~IPwm() {};

    public:  // These const funcs can't fail.
        virtual uint32_t GetChannel() const = 0;

    public:  // These functions can fail.
        virtual Status<float> SetDuty(float duty) = 0;
        virtual Status<float> SetWidthMs(float ms) = 0;
};

// A common base for the PWM.
class PwmBase : public IPwm {
    protected:
        uint32_t channel;

    public:
        PwmBase(uint32_t chan) : channel(chan) {}
        virtual ~PwmBase() override {}

        virtual uint32_t GetChannel() const final {
            return this->channel;
        }
};

// A shared PWM Reference.
typedef std::shared_ptr<IPwm> PwmRef;
typedef StatusShared<IPwm> StatusPwmRef;