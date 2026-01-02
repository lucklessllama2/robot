#pragma once

/*
 * PWM Group Interface
 *
 * This file defines the abstract interface for a Group of PWM channels.  It
 * provides functions for setting the frequency shared by the channels as
 * well as accessors to get the individual channels.
 * 
 * Since the actual PWM hardware only has so much acuracy, the set functions
 * return both a result, as well as the actual value that was applied.  For
 * example, attempting to set a PWM frequency at 200.0Hz, may result in an
 * actual freqency of 202.33 due to clock divider resolution.
 * 
 * Since the actual PWM clock may not be terribly accurate (for example, the
 * internal clock on the PCA9685 is rated at +/- 20%) it nessisary to account
 * for that inaccuracy.  Use SetAdjust to apply a "multiplier" to help account
 * for that inaccuracy.
 */

#include "pwm.h"
#include "status.h"


class IPwmGroup {
    public:
        virtual ~IPwmGroup() {};

    public:
        virtual uint32_t GetChannelCount() const = 0;

        // Return the current clock adjustment.
        virtual float GetAdjust() const = 0;

        // Get the current epsilon.
        virtual float GetEpsilon() const = 0;

        // Get the current freqency in Hz.
        virtual float GetHertz() const = 0;

    public:
        // Set the relative clock adjustment, returning result and the 
        // actual adjustment applied.
        virtual Status<float> SetAdjust(float adj) = 0;

        // Set the new epsilon, returning a result or the new actual value.
        virtual Status<float> SetEpsilon(float epsilonMs) = 0;

        // Try to set the Hertz as close as possible to the target, return
        // the result as well as the ACTUAL hertz achieved.
        virtual Status<float> SetHertz(float target) = 0;
    
        // Return a reference to a channel on this PwmGroup.
        virtual StatusPwmRef GetChannel(uint32_t cnt) = 0;
};


class PwmGroupBase : public IPwmGroup {
    protected:
        float clkAdjust;    // How much to multiply internal clock to compensate.
        float epsilonMs;    // if abs(outMs - oldMs) < epsilonMs: ignore
        float hertz;        // Freqency of pulses.
        uint32_t channels;  // Number of channels.
        PwmGroupBase(uint32_t chan) : channels(chan) {}

    public:
        virtual ~PwmGroupBase() override {}
        virtual uint32_t GetChannelCount() const final {
            return this->channels;
        }
        virtual float GetAdjust() const final {
            return this->clkAdjust;
        }
        virtual float GetEpsilon() const final {
            return this->epsilonMs;
        }
        virtual float GetHertz() const final {
            return this->hertz;
        }

        virtual Status<float> SetAdjust(float timeMs) override {
            this->epsilonMs = timeMs;
            return timeMs;
        }

        virtual Status<float> SetEpsilon(float timeMs) override {
            this->epsilonMs = timeMs;
            return timeMs;
        }
};

typedef std::shared_ptr<IPwmGroup> PwmGroupRef;
typedef StatusShared<IPwmGroup> StatusPwmGroupRef;