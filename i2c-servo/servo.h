#pragma once

#include <mutex>
#include <string>
#include <vector>

#include <stdint.h>

#include "pwm.h"

/*
 * Servo Object
 *
 * Each servo object represents a servo in the system.  A servo must be 
 * constructed from a template (ServoDef) for that type of servo which
 * provides information such as range of motion, speed, etc...   Each 
 * servo needs to be attached to a PWM which will generate the pulse
 * needed to set the servo's rotation angle.   Calling accesors or
 * running the Update function in the background will cause the servo
 * object to update the PWM to achieve the requested motion.
 * 
 * Using the API
 * 
 * Once th servo is calibrated, it's expected that the main API call
 * will be SetMotion(degs, velocity, acceleration), to request a new
 * rotation using the specified velocity and acceleration.  Or the 
 * simplified SetRotation will reuse the previous target velocity 
 * and rotation.
 * 
 * 
 * Member Naming
 *   ms<X>= milliseconds (for pulse widths).
 *   p<X> = percent of pulse width range.
 *   d<X> = degrees
 *   a<X> = acceleration in degrees per millisecond ^ 2.
 *   v<X> = velocity in degrees per millisecond.
 *
 * Calibration
 * Each servo is individually calibrated to take into account that
 * individual servo's characteristics, including servo horn alignment
 * which (using 25T horns) results in up to +/- 7degs of offset.
 * 
 * Calibration works as follows...  We set the pulse with to half way
 * betwen min and max and call that 90 degrees.  We then update the 
 * width until the servo horn itself is centered.   We then rotate the servo
 * to 0 degress and 180 degrees, adjusting the pulse width until the servo
 * horn matches that rotation.
 * 
 * This gives us pulse widths that match 3 rotations representing the
 * minimum possible, middle, an maximum possible.   We can then set a
 * arbitrary rotation by interpolating between min and mid pulse width
 * or by mid and max pulse width.
 * 
 * USE_INTERPOLATION
 * When using interpolation, two slopes are computed, one from Min to Mid 
 * points, the other from Mid to Max points.  When X is in the lower band
 * we use m1 and b1, when X is in the upper band, we use m2 and b2.  In
 * this way we interpolate between 3 known good points.
 * 
 *   Y = m1*X + b1           if (X < 50%)
 *   Y = m2*(X - 0.5) + b2   if (X > 50%)
 *   Where m1 = (pMid-pMin), b1 = pMin and m2 = (pMax - pMid), b2 = pMid 
 *   And Y = output
 * 
 * USE_GAMMA
 * When using Gamma, we create a slope between the known good outer points
 * Min and Max.   We then compute a Gamma (aka exponent) to apply to the 
 * slope to generate a curve above or below the slope to match the mid 
 * point.
 * 
 *   Y = m(X ^ Gamma) + b
 *   Where m = (pMax - pMin) and b = pMin
 */


class ServoDef {
   public:
      ServoDef(const char *name, int range, float min, float max, float v, float a);

   public:
      // Convert from a rotation percentage to a pulse width in ms.
      float PercentToPulseMs(float percent) const;

   public:
      const char *name; // Name of this servo type.
      int dRange;       // How many deg wide is this servo (180.0, 270.0)?
      float msMin;      // Absolute minimum pulse width.
      float msMax;      // Absolute maximum pulse width;
      float msDelta;    // Delta between min and max;
      float vMax;       // Absolute maximum velocity for this servo in Deg/ms
      float vMin;
      float aMax;       // Absolute maximum acceleration for this servo Deg/ms^2
      float aMin;
};

class Servo {
   protected:
      mutable bool dirty;
      mutable std::mutex mu;
      std::string name;
      const ServoDef *def;
      IPwm *pwm;
      
      // Configuration settings for this servo.
      float dMax;       // Degree at max position for calibration.
      float dMid;       // Degree at mid position for calibration.
      float dMin;       // Degree at min position for calibration.
      float dMaxCap;    // Max allowed degree when capped to avoid damage.
      float dMinCap;    // Min allowed degree when capped to avoid damage.
      float pMax;       // Percent of pulse range at max (pulse=p*msRange + msMin).
      float pMid;       // Percent of pulse range at mid.
      float pMin;       // Percent of pulse range at min.
      float pGamma;     // Calculated gamma for the curve.
      bool invert;      // Invert direction.
      bool capped;      // Using min/max caps to force dTrg/dCur range.

      // Computed helpers
      float dMinDelta;  // Degree range of lower range.
      float dMaxDelta;  // Degree range of upper range.
      float pMinDelta;  // Degree range of lower range.
      float pMaxDelta;  // Degree range of upper range.

      // Current information for this servo.
      float msCur;      // Current Pulse Width.
      float dCur;       // Degree Current position.
      float pCur;       // Percent current pulse width.
      float vCur;       // Current velocity in Deg/ms (always positive)

      float dTrg;       // Degree Target position.
      float vTrg;       // Target velocity in Deg/ms (alwasy positive)
      float aTrg;       // Current acceleration in Deg/ms^2 (always positive)

      float pTrg;       // Percent target pulse width.
      bool on;          // Servo powered on/off.
      bool moving;      // Set true, if target position != current.
      bool updating;    // Set true to enable background updates.
      bool debug;

   public:
      Servo(const ServoDef &def, IPwm *pwm);

   protected:
      // Internal helpers.
      void updateDeltas();
      void updateGamma();
      float setDegreeLocked(float deg);
      float setPercentLocked(float p);
      float setWidthMsLocked(float ms);

   public:
      bool Save(FILE *f) const;
      bool Load(FILE *f);

      /*
         * Set Absolute percentage of Pulse Range based on Servo Def.
         *   ms = p * this->def->msDelta + this->def->msMin
         * 
         * NOTE:  Use with caution, this bypasses motion limits so it can
         *    damange the model if not used carefully.   This should only
         *    be used during tuning to calibrate the servo prior to install.
         *
         */
      float SetPercentAbs(float p);
      float GetPercentAbs() const   { return pCur; }

      float SetDegreeAdj(float d);

      bool SetOn(bool power);

   public:
      // Configuration tuning Parameters
      void SetMaxParam(float deg, float pct);
      void GetMaxParam(float &deg, float &pct) const { deg = dMax; pct = pMax; }

      void SetMinParam(float deg, float pct);
      void GetMinParam(float &deg, float &pct) const { deg = dMin; pct = pMin; }

      void SetMidParam(float deg, float pct);
      void GetMidParam(float &deg, float &pct) const { deg = dMid; pct = pMid; }

      void SetDegCap(float min, float max);
      void GetDegCap(float &min, float &max) const { min = dMinCap; max = dMaxCap; }

   public:
      // Update current position and velocity based on elapsed time.
      bool Update(float t);
      void SetUpdating(bool en) { updating = en; }
      bool GetUpdating() const  { return updating; }

      // Change the name.
      const std::string& GetName() const { return name; }
      void SetName(const std::string& n) { name = n;  }

      // Set the PWM for this servo.
      IPwm *GetPWM() const { return pwm; }
      void SetPwm(IPwm *p) { pwm = p; }

      // Set true to Debug on SetDegree update.
      bool GetDebug() const     { return debug; }
      void SetDebug(bool i)     { debug = i; }

      // Set true to INVERT degree and PWM relationship (change rotation direction).
      bool GetInvert() const     { return invert; }
      void SetInvert(bool i)     { invert = i; }

      // Enable MIN, MAX rotation constraints.
      bool GetCapped() const     { return capped; }
      void SetCapped(bool c)     { capped = c; }

      // Return degree capped by motion limit (if capped=true) or default 0.0-180.0
      float CapDegree(float deg);
      float CapVelocity(float v);
      float CapAccel(float a);

      // Set rotation target using the current velocity and accel.
      float SetRotation(float deg);

      // Set the current velocity to apply when moving in degrees per millisecond.
      float SetVelocity(float dps);

      // Set the acceleration to apply when moving in degree per second per millisecond.
      float SetAccel(float dpss);

      // The the new motion target including rotation, velocity, and acceleration.
      // NOTE: Calling SetMotion, will first zero the current velocity.
      float SetMotion(float deg, float v, float a);

      // Get the current rotation, velocity and acceleration.
      // NOTE: Returns false if the servo is not moving, in which case the velocity 
      //       and acceleration are the last used values or targets.
      bool GetCurrent(float &deg, float &v, float &a) const {
         deg = dCur;
         v = vCur;
         a = aTrg;
         return moving;
      }

      // Get the target rotation, velocity and acceleation.
      // NOTE: Returns false if not moving, however the target values will be applied
      //       on the next call to SetRotation.
      bool GetTarget(float &deg, float &v, float &a) const {
         deg = dTrg;
         v = vTrg;
         a = aTrg;
         return moving;
      }
   public:
      bool IsMoving() const   { return moving; }
      bool IsDirty() const    { return dirty;  }
};


//SinceCam Servos
extern const ServoDef SC_EX2940MGX;
extern const ServoDef SC_EX2927MGX;

//DSSERVO Servos
extern const ServoDef DS_3245SG;

#define SERVO_DEF180(_name,_min, _max, _v, _a) \
   const ServoDef _name(# _name, 180, _min, _max, _v, _a)

#define SERVO_DEF270(_name, _min, _max, _v, _a) \
   const ServoDef _name(# _name, 270, _min, _max, _v, _a)


void ServosInit(void);
void ServosDump(void);
bool ServosSave(void);
bool ServosLoad(void);
void ServosEnableUpdating(bool en);

Servo *ServosGet(int group, int channel);

