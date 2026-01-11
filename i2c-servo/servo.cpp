#include <algorithm>
#include <atomic>
#include <cmath>
#include <thread>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "pca9685.h"
#include "pwm_group.h"
#include "pwm_mock.h"
#include "servo.h"


SERVO_DEF270(SC_EX2927MGX, 0.5, 2.5, 30.0, 60.0);
SERVO_DEF180(SC_EX2940MGX, 0.5, 2.5, 30.0, 60.0);
SERVO_DEF270(DS_3245SG, 0.5, 2.5, 30.0, 60.0);

#define EPSILON_DEG   0.01
#define EPSILON_GAMMA 0.0001
#define COUNT_GAMMA   16


ServoDef::ServoDef(const char *name, int range, float min, float max, float v, float a) {
    this->name = name;
    this->dRange = range;
    this->msMin = min;
    this->msMax = max;
    this->msDelta = max - min;
    this->vMax = v / 1000.0f; // Convert from Deg/s to Deg/ms
    this->vMin = this->vMax / 20.f;
    this->aMax = a / 1000.0f; // Convert from Deg/s to Deg/ms
    this->aMin = this->aMax / 10.f;
}

Servo::Servo(const ServoDef &def, IPwm *pwm) {
    this->def = &def;
    this->pwm = pwm;
    this->invert = false;

    // Set allowed range for all servos to 180 deg centered at 90.
    this->dMin = 0.0;
    this->dMax = 180.0;
    this->dMid = 90.0;
 
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
 
    // Generate deltas.
    updateDeltas();

    // By default enforce 15% movement cap to prevent servo from 
    // breaking model.  We expect to load real caps.
    this->capped = true;
    this->dMinCap = this->dMid - this->dMinDelta * 0.15;
    this->dMaxCap = this->dMax - this->dMaxDelta * 0.15;

    // Clear all movement data;
    this->dCur = 89.0;
    this->vCur = 0.0;
    this->vTrg = 0.0;
    this->moving = true;
    SetMotion((this->dMinCap + this->dMaxCap) / 2.0, this->def->vMin, this->def->aMax);
}

void Servo::updateGamma() {
    float gamma = 1.0;
    float range = this->pMax - this->pMin;
    float adjust = 0.5;
    float delta;

    printf("Compute Gamma for %f, %f, %f\n", dMin, dMid, dMax);
    for (int a=0; a < COUNT_GAMMA; a++) {
        float gval = range * powf(0.5, gamma) + dMin;
        delta = dMid - gval;
        if (std::abs(delta) < EPSILON_GAMMA) {
            break;
        }
        // If computer value is too high, pull curve down.
        if (delta > 0.0f) {
            gamma -= adjust;
        } else {
            gamma += adjust;
        }
        adjust /= 2.0f;
    }
    this->pGamma = gamma;
}


void Servo::updateDeltas() {
    this->dMaxDelta = this->dMax - this->dMid;
    this->pMaxDelta = this->pMax - this->pMid;
    this->dMinDelta = this->dMid - this->dMin;
    this->pMinDelta = this->pMid - this->pMin;
}

void Servo::SetMaxParam(float deg, float pct) {
    assert(deg >= -5.0f);
    assert(deg <= 185.0f);
    assert(pct >= -0.02f);
    assert(pct <= 1.02f);

    this->dirty = true;
    this->dMax = deg;
    this->pMax = pct;
    updateDeltas();
}

void Servo::SetMidParam(float deg, float pct) {
    assert(deg >= -5.0f);
    assert(deg <= 185.0f);
    assert(pct >= -0.02f);
    assert(pct <= 1.02f);

    this->dirty = true;
    this->dMid = deg;
    this->pMid = pct;
    updateDeltas();
}

void Servo::SetMinParam(float deg, float pct) {
    assert(deg >= -5.0f);
    assert(deg <= 185.0f);
    assert(pct >= -0.02f);
    assert(pct <= 1.02f);
    this->dirty = true;
    this->dMin = deg;
    this->pMin = pct;
    updateDeltas();
}

void Servo::SetDegCap(float min, float max) {
    assert(min >= 0.0f);
    assert(max <= 180.0f);
    assert(min < max);
    if (min != this->dMinCap || max != this->dMaxCap) {
        this->dMinCap = min;
        this->dMaxCap = max;
        this->dirty = true;
    }
}

bool Servo::Save(FILE *f) const {
    fprintf(f, "SRV_ID: %s\n", this->name.c_str());
    fprintf(f, "DEGREE: %f, %f, %f\n", this->dMin, this->dMid, this->dMax);
    fprintf(f, "PCT_ms: %f, %f, %f\n", this->pMin * 100.0, this->pMid * 100.0, this->pMax * 100.0);
    fprintf(f, "DEGCAP: %f, %f\n", this->dMinCap, this->dMaxCap);
    fprintf(f, "INVERT: %d\n", this->invert ? 1 : 0);
    fprintf(f, "\n");
    this->dirty = false;
    return true;
}

bool Servo::Load(FILE *f) {
    char line[180];
    char name[80];
    int invert;
    int result;

    assert(fgets(line, sizeof(line), f) != nullptr);
    result = sscanf(line, "SRV_ID: %s\n", name);
    assert(result == 1);
    this->name = name;

    result = fscanf(f, "DEGREE: %f, %f, %f\n", &this->dMin, &this->dMid, &this->dMax);
    assert(result == 3);

    result = fscanf(f, "PCT_ms: %f, %f, %f\n", &this->pMin, &this->pMid, &this->pMax);
    assert(result == 3);
    this->pMin /= 100.0;
    this->pMid /= 100.0;
    this->pMax /= 100.0;

    result = fscanf(f, "DEGCAP: %f, %f\n", &this->dMinCap, &this->dMaxCap);
    assert(result == 2);

    result = fscanf(f, "INVERT: %d\n", &invert);
    assert(result == 1);

    this->invert == (invert != 0);
    this->dirty = false;
    updateDeltas();
    updateGamma();

    SetMotion((this->dMinCap + this->dMaxCap) / 2.0, this->def->vMin, this->def->aMax);
    return true;
}

float Servo::setWidthMsLocked(float ms) {
    if (this->on)
        this->pwm->SetWidthMs(ms);
    else
        this->pwm->SetWidthMs(0.0);
    this->msCur = ms;
    return ms;
}

float Servo::setPercentLocked(float pct) {
    float oldMs = this->msCur;
    float oldPct = this->pCur;

    float ms = this->def->msDelta * pct + this->def->msMin;
    setWidthMsLocked(ms);
    this->pCur = pct;
    return pct;
}

float Servo::setDegreeLocked(float deg) {
    float pct;

    
    if (this->debug) {
        this->dCur != deg;
        printf("%s changing %f to %f.\n", this->name.c_str(), this->dCur, deg);
    }

    deg = CapDegree(deg);
#ifdef USE_GAMMA
    float dPrt = deg / 180.0f;                          // Convert degrees to percent rotation.
    float range = this->pMax - this->pMin;              // Calulate m;
    pct = range * pow(dPct, this->gamma) + this->pMin;  // y = m*(X^gamma) + b;
#else // End GAMMA start INTERPOLATION
    float scale;
    if (deg > this->dMid) {
        scale = (deg - this->dMid) / this->dMaxDelta;
        pct = (scale * this->pMaxDelta) + this->pMid;
    } else {
        scale = (deg - this->dMin) / this->dMinDelta;
        pct = (scale * this->pMinDelta) + this->pMin;
    }
    setPercentLocked(pct);
    return deg;
#endif // End INTERPOLATION
}
bool Servo::SetOn(bool power) {
    std::lock_guard<std::mutex> lock(this->mu);
    this->on = power;
    if (!power) {
        // Turn off the servo PWM.
        this->pwm->SetWidthMs(0.0);
    } else {
        // Restore current position.
        setDegreeLocked(this->dCur);
    }
    return this->on;
}
float Servo::SetDegreeAdj(float deg) {
    std::lock_guard<std::mutex> lock(this->mu);
    return setDegreeLocked(deg);
}

float Servo::SetPercentAbs(float pct) {
    std::lock_guard<std::mutex> lock(this->mu);
    return setPercentLocked(pct);
}

bool Servo::Update(float ms) {
    // Only apply updates if running.
    if (this->updating == false) {
        return true;
    }

    std::lock_guard<std::mutex> lock(this->mu);

    // If we are within epsilon of the requested rotation, then we are
    // done moving.
    if (std::abs(this->dTrg - this->dCur) < EPSILON_DEG) {
        this->moving = false;
        this->dCur = dTrg;
        this->vCur = 0;
        return true;
    }

    // If we haven't hit target velocity, then update it.
    float aDelta = this->aTrg * ms;
    if (this->vCur < this->vTrg) {
        this->vCur = std::min(this->vCur + aDelta, this->vTrg);
    }

    // If we are before the target degree, then add velocity.
    float dDelta = this->vCur * ms;
    if (this->dCur < this->dTrg) {
        this->dCur = std::min(this->dCur + dDelta, this->dTrg);
    } else {
        this->dCur = std::max(this->dCur - dDelta, this->dTrg);
    }

    setDegreeLocked(this->dCur);
    return false;
}

float Servo::CapDegree(float deg) {
    if (capped) {
        // Cap to provided movement limits.
        deg = std::max(this->dMinCap, deg);
        deg = std::min(this->dMaxCap, deg);
    } else{
        // Cap to default movement limits.
        deg = std::max(0.0f, deg);
        deg = std::min(180.0f, deg);
    }
    return deg;
}

float Servo::CapVelocity(float v) {
    v = std::max(this->def->vMin, v);
    v = std::min(this->def->vMax, v);
    return v;
}

float Servo::CapAccel(float a) {
    a = std::max(this->def->aMin, a);
    a = std::min(this->def->aMax, a);
    return a;
}

float Servo::SetRotation(float deg) {
    return SetMotion(deg, this->vTrg, this->aTrg);
}

float Servo::SetVelocity(float v) {
    this->vTrg = CapVelocity(v);
    return this->vTrg;
}

float Servo::SetAccel(float a) {
    this->aTrg = CapAccel(a);
    return this->aTrg;
}

float Servo::SetMotion(float deg, float v, float a) {
    std::lock_guard<std::mutex> lock(this->mu);
    deg = CapDegree(deg);
    v = CapVelocity(a);
    a = CapAccel(a);

    bool needsMove = false;
    if (this->dTrg != deg) {
        this->dTrg = deg;
        this->moving = true;
    }

    this->vTrg = v;
    this->aTrg = a;
    this->vCur = 0.0;
    return deg;
}

Servo *servos[NUM_GROUPS][NUM_SERVOS];

static void servoLoop(void) {
    auto previous = std::chrono::high_resolution_clock::now();
    while(1) {
        usleep(2000);
        auto current = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(current - previous);
        float fdelta = (float) delta.count() / 1000.0f;
        for (int b=0; b < NUM_GROUPS; b++) {
            for (int a=0; a < NUM_SERVOS; a++) {
                servos[b][a]->Update(fdelta);
            }
        }
        previous = current;
    }
}

void ServosInit(void) {
    std::vector<IPwmGroup*> groups;

#ifdef x86_64
    auto pca = NewPwmMock();
    printf("Using MOCK PWMs\n");
#else
    auto pca = NewPca9685("/dev/i2c-1", 0x40);
    printf("Using PCA9685\n");
#endif    
    groups.push_back(pca.Value());

    for (int b=0; b < NUM_GROUPS; b++) {
        IPwmGroup *group = groups[b];
        for (int a=0; a < NUM_SERVOS; a++) {
            auto pwm = group->GetChannel(a);
            servos[b][a] = new Servo(DS_3245SG, pwm.Value());
        }
    }

    ServosLoad();

    std::thread servo_thread(servoLoop);
    servo_thread.detach();
}


bool ServosSave(void) {
    FILE *fd = fopen("servo.txt", "wt");
    if (fd == nullptr) {
        perror("Failed to create servo data.\n");
        return false;
    }
    for (int b=0; b < NUM_GROUPS; b++) {
        for (int a=0; a < NUM_SERVOS; a++) {
            servos[b][a]->Save(fd);
        }
    }
    fclose(fd);
    printf("Servos saved.\n");
    return true;
}

bool ServosLoad(void) {
    FILE *fd = fopen("servo.txt", "rt");
    if (fd == nullptr) {
        perror("Failed to open servo data.\n");
        return false;
    }
    for (int b=0; b < NUM_GROUPS; b++) {
        for (int a=0; a < NUM_SERVOS; a++) {
            servos[b][a]->Load(fd);
        }
    }
    fclose(fd);
    printf("Servos loaded.\n");
    return true;
}

void ServosDump(void) {
    for (int b=0; b < NUM_GROUPS; b++) {
        for (int a=0; a < NUM_SERVOS; a++) {
            servos[b][a]->Save(stdout);
        }
    }
}

Servo *ServosGet(int group, int chan) {
    assert(group >= 0);
    assert(group < NUM_GROUPS);
    assert(chan >= 0);
    assert(chan < NUM_SERVOS);

    return servos[group][chan];
}

void ServosEnableUpdate(bool en) {
    for (int b=0; b < NUM_GROUPS; b++) {
        for (int a=0; a < NUM_SERVOS; a++) {
            servos[b][a]->SetUpdating(en);
        }
    }
}