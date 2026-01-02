#pragma once

#include <fstream>
#include <mutex>
#include <string>

#include <stdint.h>

#include "pwm.h"

/*
 * Servo Object
 *
 * Each servo object represents a servo in the system.  A servo must be 
 * constructed from a template (ServoDef) for that type of servo which
 * provides information such as range of motion, speed, etc... 
 * 
 * To use a servo, you must first set it's PWM.  The servo object will
 * be responsible for updating the attached PWM to achieve the desired
 * location and speed.  The servo will do this automatically in the
 * background by calling Update with the time delta between from the
 * last Update call.
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
      float aMax;       // Absolute maximum acceleration for this servo Deg/ms^2
 };

 class Servo {
   protected:
      mutable bool dirty;
      mutable std::mutex mu;
      std::shared_ptr<IPwm> pwm;
      std::string name;
      const ServoDef *def;
      
      // Configuration settings for this servo.
      float dMax;       // Degree at max position.
      float dMid;       // Degree at mid position.
      float dMin;       // Degree at min position.
      float pMax;       // Percent of pulse range at max. (pulse=p*msRange + msMin)
      float pMid;       // Percent of pulse range at mid.
      float pMin;       // Percent of pulse range at min.
      float dMinDelta;  // Degree range of lower range.
      float dMaxDelta;  // Degree range of upper range.
      float pMinDelta;  // Degree range of lower range.
      float pMaxDelta;  // Degree range of upper range.
      bool invert;      // Invert direction.

      // Motion information for this servo.
      float msCur;      // Current Pulse Width.
      float dCur;       // Degree Current position.
      float dTrg;       // Degree Target position.
      float pCur;       // Percent current.
      float pTrg;       // Percent target.
      float vCur;       // Current velocity in Deg/ms (always positive)
      float vTrg;       // Target velocity.
      float aCur;       // Current acceleration in Deg/ms^2 (always positive)
      bool moving;      // Set true, if target position != current.

   public:
      Servo(const ServoDef &def, std::shared_ptr<IPwm> pwm);

   public:
      bool Save(int fd) const;
      bool Load(int fd);

      float SetWidthMs(float ms);
      float SetPercentAbs(float p);
      float SetDegreeAdj(float d);

      void SetMaxParam(float deg, float pct);
      void SetMinParam(float deg, float pct);
      void SetMidParam(float deg, float pct);

   public:
      // Update current position and velocity, returning True if already at target.
      //bool Update(float t);

      // Change the name.
      void SetName(const std::string& name);

      // Set the PWM for this servo.
      void SetPwm(std::shared_ptr<IPwm> pwm);

      // Set true to reverse servo rotation direction.
      void SetInvert(bool inv);


#if 0
      // Set the target position, getting there as quickly as possible.
      bool SetTarget(float d);

      // Set the target position, rotating at given velocity.
      bool SetVelocity(float d, float v);

      // Set movement parameters: position, velocity, and accel.
      bool SetMovement(float p, float v, float a);
#endif

   public:
      bool  IsMoving() const;
      bool  IsDirty() const;
      float GetTarget() const;
      float GetPosition() const;
      float GetVelocity() const;
      float GetAccel() const;
      float GetPercent() const;

      float GetMinDegree() const;
      float GetMaxDegree() const;
      float GetMidDegree() const;
      float GetMinPercent() const;
      float GetMaxPercent() const;
      float GetMidPercent() const;
 };


//SinceCam Servos
extern const ServoDef SC_EX2940MGX;
extern const ServoDef SC_EX2927MGX;
extern const ServoDef DS_3245SG;

#define SERVO_DEF180(_name,_min, _max, _v, _a) \
   const ServoDef _name(# _name, 180, _min, _max, _v, _a)

#define SERVO_DEF270(_name, _min, _max, _v, _a) \
   const ServoDef _name(# _name, 270, _min, _max, _v, _a)
