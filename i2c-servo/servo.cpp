#include <algorithm>
#include <cmath>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "servo.h"




SERVO_DEF270(SC_EX2927MGX, 0.5, 2.5, 30.0, 60.0);
SERVO_DEF180(SC_EX2940MGX, 0.5, 2.5, 30.0, 60.0);
SERVO_DEF270(DS_3245SG, 0.5, 2.5, 30.0, 60.0);

// Delta in degree before deciding they are equal.
#define EPSILON 0.001

ServoDef::ServoDef(const char *name, int range, float min, float max, float v, float a) {
    this->name = name;
    this->dRange = range;
    this->msMin = min;
    this->msMax = max;
    this->msDelta = max - min;
    this->vMax = v / 1000.0f; // Convert from Deg/s to Deg/ms
    this->aMax = a / 1000.0f; // Convert from Deg/s to Deg/ms
}

Servo::Servo(const ServoDef &def, std::shared_ptr<IPwm> pwm) {
    this->def = &def;
    this->pwm = pwm;
    this->invert = false;

    // Set allowed range for all servos to 180 deg centered at 90.
    this->dMin = 0.0;
    this->dMax = 180.0;
    this->dMid = 90.0;
    this->dMaxDelta = 90.0;
    this->dMinDelta = 90.0;

    // Set pulse widths based on range for this servo.
    if (def.dRange == 180) {
        this->pMin = 0.0f;
        this->pMax = 1.0f;
        this->pMid = 0.5f;
    } else {
        this->pMin = 0.16f;
        this->pMax = 0.84f;
        this->pMid = 0.5f;
    }
    this->pMaxDelta = this->pMax - this->pMid;
    this->pMinDelta = this->pMid - this->pMin;

    // Clear all movement data;
    this->vCur = 0.0;
    this->vTrg = 0.0;
    this->aCur = 0.0;

    SetDegreeAdj(90.0);
}

void Servo::SetMaxParam(float deg, float pct) {
    assert(deg >= -5.0f);
    assert(deg <= 185.0f);
    assert(pct >= -0.02f);
    assert(pct <= 1.02f);

    this->dirty = true;
    this->dMax = deg;
    this->pMax = pct;
    this->dMaxDelta = this->dMax - this->dMid;
    this->pMaxDelta = this->pMax - this->pMid;
}

void Servo::SetMidParam(float deg, float pct) {
    assert(deg >= -5.0f);
    assert(deg <= 185.0f);
    assert(pct >= -0.02f);
    assert(pct <= 1.02f);
    this->dirty = true;
    this->dMid = deg;
    this->pMid = pct;
    this->dMaxDelta = this->dMax - this->dMid;
    this->pMaxDelta = this->pMax - this->pMid;
    this->dMinDelta = this->dMid - this->dMin;
    this->pMinDelta = this->pMid - this->pMin;
    printf("SET: %f %f\n", this->dMid, this->pMid);
}

void Servo::SetMinParam(float deg, float pct) {
    assert(deg >= -5.0f);
    assert(deg <= 185.0f);
    assert(pct >= -0.02f);
    assert(pct <= 1.02f);
    this->dirty = true;
    this->dMin = deg;
    this->pMin = pct;
    this->dMinDelta = this->dMid - this->dMin;
    this->pMinDelta = this->pMid - this->pMin;
}

#define SaveMember(x) write(fd, &x, sizeof(x))
bool Servo::Save(int fd) const {
    SaveMember(this->dMax);
    SaveMember(this->dMid);
    SaveMember(this->dMin);
    SaveMember(this->dMinDelta);
    SaveMember(this->dMaxDelta);
    SaveMember(this->pMax);
    SaveMember(this->pMid);
    SaveMember(this->pMin);
    SaveMember(this->pMinDelta);
    SaveMember(this->pMaxDelta);
    SaveMember(this->invert);
    printf("SAVE: %f %f\n", this->dMid, this->pMid);
    this->dirty = false;
    return true;
}

#define LoadMember(x) read(fd, &x, sizeof(x))
bool Servo::Load(int fd) {
    LoadMember(this->dMax);
    LoadMember(this->dMid);
    LoadMember(this->dMin);
    LoadMember(this->dMinDelta);
    LoadMember(this->dMaxDelta);
    LoadMember(this->pMax);
    LoadMember(this->pMid);
    LoadMember(this->pMin);
    LoadMember(this->pMinDelta);
    LoadMember(this->pMaxDelta);
    LoadMember(this->invert);
    printf("LOAD: %f %f\n", this->dMid, this->pMid);
    this->dirty = false;
    return true;
}

void Servo::SetPwm(std::shared_ptr<IPwm> pwm) {
    this->pwm = pwm;
}

float Servo::SetWidthMs(float ms) {
    this->pwm->SetWidthMs(ms);
    this->msCur = ms;
    return ms;
}

float Servo::SetPercentAbs(float pct) {
    this->pCur = pct;
    this->pTrg = pct;
    float ms = this->def->msDelta * pct + this->def->msMin;
    SetWidthMs(ms);
    return pct;
}

float Servo::SetDegreeAdj(float deg) {
    float scale;
    float range;
    float pct;
    deg = std::min(deg, this->dMax);
    deg = std::max(deg, this->dMin);
    if (deg > this->dMid) {
        scale = (deg - this->dMid) / this->dMaxDelta;
        pct = (scale * this->pMaxDelta) + this->pMid;
    } else {
        scale = (deg - this->dMin) / this->dMinDelta;
        pct = (scale * this->pMinDelta) + this->pMin;
    }
    SetPercentAbs(pct);
    return pct;
}

#if 0
extern volatile bool debug;
bool Servo::Update(float ms) {
    bool doPrint = debug && (this->pwm->GetChannel() == 3) && 0;
    std::lock_guard<std::mutex> lock(this->mu);
    char dir = '+';
    float pDelta;
    float old = this->pCur;
    if (this->pCur == this->pTrg) {
        this->vCur = 0.0;
        this->aCur = 0.0;
        if (this->moving) {
            this->moving = false;
            return true;
        }
    } else {
        // If we haven't hit target velocity, then update it.
        float vDelta = this->aCur * ms;
        if (this->vCur < this->vTrg) {
            this->vCur = std::min(this->aCur + vDelta, this->vTrg);
        } 

        // Apply the current veloticy to the current position.
        pDelta = this->vCur * ms;
        if (this->pCur < this->pTrg) {
            this->pCur = std::min(this->pCur + pDelta, this->pTrg);
        } else {
            this->pCur = std::max(this->pCur - pDelta, this->pTrg);
            dir = '-';
        }
    }

    // Calculate the new rotation percentage.
    float range = (this->pMax - this->pMin);
    float percent = (this->pCur - this->pMin) / range;

    if (doPrint) {
        printf("%c O=%f, T=%f, N=%f, P=%f\n", dir, old, this->pTrg, this->pCur, percent);
    }
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
#endif

// Set true to reverse servo rotation direction.
void Servo::SetInvert(bool inv) {
    std::lock_guard<std::mutex> lock(this->mu);
}

#if 0
bool Servo::SetTarget(float p) {
    return this->SetMovement(p, this->def->vMax, this->def->aMax);
}

bool Servo::SetVelocity(float p, float v) {
    return this->SetMovement(p, v, this->def->aMax);
}

bool Servo::SetMovement(float d, float v, float a) {
    bool modified = false;
    std::lock_guard<std::mutex> lock(this->mu);
    float delta = std::abs(this->pCur - p);

    // Move at least one frame, to ensure PWM is correct.
    this->moving = true;

    // If we are already there, then don't do anything.
    if (delta < EPSILON) {
        this->pCur = p;
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
#endif


bool  Servo::IsMoving() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->moving;
}

bool  Servo::IsDirty() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->dirty;
}

float Servo::GetTarget() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->pTrg;
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

float Servo::GetMaxDegree() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->dMax;
}

float Servo::GetMidDegree() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->dMid;
}

float Servo::GetMinDegree() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->dMin;
}

float Servo::GetMaxPercent() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->pMax;
}

float Servo::GetMidPercent() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->pMid;
}

float Servo::GetMinPercent() const {
    std::lock_guard<std::mutex> lock(this->mu);
    return this->pMin;
}

