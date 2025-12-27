#pragma once

#include <mutex>

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
      ServoDef(const char *name, float range, float min, float max, float g, float v, float a);

   public:
      // Convert from a rotation percentage to a pulse width in ms.
      float PercentToPulseMs(float percent) const;

   public:
      const char *name; // Name of this servo type.
      float range;      // How many deg wide is this servo?
      float msMin;      // Minimum pulse width;
      float msMax;      // Maximum pulse width;
      float msDelta;    // Max - Min;
      float gamma;      // The "non-linearity" of the curve for pulse to position.
      float vMax;       // Maximum velocity for this servo in Deg/ms
      float aMax;       // Maximum acceleration for this servo Deg/ms^2
 };

 class Servo {
   protected:
      mutable std::mutex mu;
      std::shared_ptr<IPwm> pwm;
      const ServoDef *def;
      const char *name;

      bool invert;      // Invert direction.
      bool moving;      // Set true, if target position != current.
      float pOff;       // Set position offset (trim).
      float pMin;       // Minimum position.
      float pMax;       // Maximum position.
      float pCur;       // Current position.
      float pTrg;       // Target position.
      float vCur;       // Current velocity in Deg/ms (always positive)
      float vTrg;       // Target velocity.
      float aCur;       // Current acceleration in Deg/ms^2 (always positive)

   public:
      Servo(const char *name, const ServoDef *def);
   
   public:
      // Update current position and velocity, returning True if already at target.
      bool Update(float t);

      // Set the trim to make the current position the middle.
      void  Zero();

      // Set the PWM for this servo.
      void SetPwm(std::shared_ptr<IPwm> pwm);

      // Min and Max positions in Deg, (max - min) < def->range.
      bool SetRange(float min, float max);

      // Set true to reverse servo rotation direction.
      void SetInvert(bool inv);

      // Set the target position, getting there as quickly as possible.
      bool SetPosition(float p);

      // Set the target position, rotating at given velocity.
      bool SetTarget(float p, float v);

      // Set movement parameters: position, velocity, and accel.
      bool SetMovement(float p, float v, float a);

   public:
      bool  IsMoving() const;
      float GetPosition() const;
      float GetVelocity() const;
      float GetAccel() const;
 };


//SinceCam Servos
extern const ServoDef SC_EX2940MGX;
extern const ServoDef SC_EX2927MGX;

#define SERVO_DEF180(_name,_min, _max, _g, _v, _a) \
   const ServoDef _name(# _name, 180.0, _min, _max, _g, _v, _a)

#define SERVO_DEF270(_name, _min, _max, _g, _v, _a) \
   const ServoDef _name(# _name, 270.0, _min, _max, _g, _v, _a)
