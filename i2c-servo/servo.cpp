#include <algorithm>
#include <cmath>

#include <stdio.h>

#include "servo.h"




SERVO_DEF270(SC_EX2927MGX, 0.5, 2.5, 1.0, 30.0, 60.0);
SERVO_DEF180(SC_EX2940MGX, 0.5, 2.5, 1.0, 30.0, 60.0);

// Delta in degree before deciding they are equal.
#define EPSILON 0.001

ServoDef::ServoDef(const char *name, float range, float min, float max, float g, float v, float a) {
    this->name = name;
    this->range = range;
    this->msMin = min;
    this->msMax = max;
    this->msDelta = max - min;
    this->gamma = g;
    this->vMax = v / 1000.0f; // Convert from Deg/s to Deg/ms
    this->aMax = a / 1000.0f; // Convert from Deg/s to Deg/ms
}

float ServoDef::PercentToPulseMs(float percent) const {
    percent = std::max(percent, (float) 0.0);
    float percent_gamma = std::pow(percent, this->gamma);
    float pulse = (percent_gamma * this->msDelta) + this->msMin;
    float capped = std::min(pulse, this->msMax);
    return capped;
}


Servo::Servo(const char *name, const ServoDef *def) {
    this->name = name;
    this->def = def;
    this->pOff = 0.0;
    this->pMin = 0.0;
    this->pMax = def->range;
    this->pCur = 0;
    this->pTrg = def->range / 2.0;
    this->vCur = 0.0;
    this->vTrg = 0.0;

    SetMovement(def->range / 2.0, def->vMax, def->aMax);
}


void Servo::SetPwm(std::shared_ptr<IPwm> pwm) {
    this->pwm = pwm;
}

bool Servo::Update(float ms) {
    std::lock_guard<std::mutex> lock(this->mu);
    if (this->pCur == this->pTrg) {
        this->vCur = 0.0;
        this->aCur = 0.0;
        if (this->moving) {
            this->moving = false;
            printf("Done moving %s\n", this->name);
            return true;
        }
    } else {
        // If we haven't hit target velocity, then update it.
        float vDelta = this->aCur * ms;
        if (this->vCur < this->vTrg) {
            this->vCur = std::min(this->aCur + vDelta, this->vTrg);
        } 

        // Apply the current veloticy to the current position.
        float pDelta = this->vCur * ms;
        if (this->pCur < this->pTrg) {
            this->pCur = std::min(this->pCur + pDelta, this->pTrg);
        } else {
            this->pCur = std::max(this->pCur - pDelta, this->pTrg);
        }
    }

    // Calculate the new rotation percentage.
    float range = (this->pMax - this->pMin);
    float percent = (this->pCur - this->pMin) / range;

    // If this servo rotation is inverted, then flip the rotation percentage.
    if (this->invert) {
        percent = 1.0 - percent;
    }

    float pulse = this->def->PercentToPulseMs(percent);
    this->pwm->SetWidthMs(pulse);
    return false;
}

// Min and Max positions in Deg, (max - min) < def->range.
bool Servo::SetRange(float min, float max) {
    std::lock_guard<std::mutex> lock(this->mu);
    return true;
}

// Set true to reverse servo rotation direction.
void Servo::SetInvert(bool inv) {
    std::lock_guard<std::mutex> lock(this->mu);
}

bool Servo::SetPosition(float p) {
    return this->SetMovement(p, this->def->vMax, this->def->aMax);
}

bool Servo::SetTarget(float p, float v) {
    return this->SetMovement(p, v, this->def->aMax);
}

bool Servo::SetMovement(float p, float v, float a) {
    bool modified = false;
    std::lock_guard<std::mutex> lock(this->mu);
    float delta = std::abs(this->pCur - p);

    // Move at least one frame, to ensure PWM is correct.
    this->moving = true;

    // If we are already there, then don't do anything.
    if (delta < EPSILON) {
        this->pCur = p;
        this->pTrg = p;
        return false;
    }

    if (p < this->pMin) {
        modified = true;
        p = this->pMin;
    }
    if (p > this->pMax) {
        modified = true;
        p = this->pMax;
    }
    if (v < EPSILON) {
        modified = true;
        v = EPSILON;
    }
    if (v > this->def->vMax) {
        modified = true;
        v = this->def->vMax;
    }
    if (a < EPSILON) {
        modified = true;
        a = EPSILON;
    }
    if (a > this->def->aMax) {
        modified = true;
        a = this->def->aMax;
    }
    // If the new direction is the same, keep velocity
    if (((this->pCur < this->pTrg) && (this->pCur < p)) ||
        ((this->pCur > this->pTrg) && (this->pCur > p))) {
        this->vCur = std::min(this->vCur, v);
    } else {
        this->vCur = 0.0;
    }

    // Set targets position, velocity, and accel.
    this->pTrg = p;
    this->vTrg = v;
    this->aCur = a;
    return modified;
}

bool  Servo::IsMoving() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->moving;
}

float Servo::GetPosition() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->pCur;
}

float Servo::GetVelocity() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->vCur;
}

float Servo::GetAccel() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->aCur;
}