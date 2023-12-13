/*
  MMBlaster.h - Library for Monkee Mods Blaster
  Created by sasaug
*/
#ifndef MMBlaster_h
#define MMBlaster_h

#include "Arduino.h"
#include "DigitalPin.h"

#define MMBLASTER_VERSION "0.1.0"

enum BlasterType{
  FLYWHEEL, BRUSHED_FLYWHEEL, AEB
};

enum PusherType{
  TIMING_MOTOR, TIMING_SOLENOID, LIMITER
};

enum MotorMode{
  COOL, RAGE
};

enum FiringMode{
  SAFE, SEMI, BURST1, BURST2, FULLAUTO
};

enum TriggerBehavior{
  PASSIVE, REACTIVE
};

enum MotorSpeed{
  STOP, CRUISE, MAX
};

class MMBlaster{
  public:
    typedef void (*cbfunc_a) (MotorSpeed speed, bool isFired);
    typedef void (*cbfunc_b) (MotorSpeed speed);

	  MMBlaster(BlasterType type);
    void loop();
    void triggerPress();
    void triggerRelease();
    void revPress();
    void revRelease();


    void setFiringMode(FiringMode mode);
    void setMotorMode(MotorMode mode);
    void setTriggerDelay(long delay);
    void setMotorShutoffDelay(long delay);
    void setExtraFiringCycleDelay(long delay);
    void setFiringRate(int count);
    void setBurst1Amount(int value);
    void setBurst2Amount(int value);
    void setFullAutoMaxAmount(int value);
    void setSemiAutoTriggerBehavior(TriggerBehavior behavior);
    void setBurstfireTriggerBehavior(TriggerBehavior behavior);
    void setPusherCallback(cbfunc_a callbackFunction);
    void setMotorCallback(cbfunc_b callbackFunction);

    void setPusherType(PusherType type);
    void attachPusherLimiterPin(DigitalPin pin);
    void setPusherLimiterMaxTime(long maxTime);
    void setRevTriggerEnabled(bool enable);
    void setRageOnRev(bool enable);
    void setFiringShotCycle(int shotCycle);

    FiringMode getFiringMode();
    MotorMode getMotorMode();
    bool isBlasterFiring();

  private:
    void callbackMotor(MotorSpeed speed);
    void callbackPusher(MotorSpeed speed, bool isFired);

    BlasterType blasterType;
    FiringMode firingMode = SAFE;
    PusherType pusherType = TIMING_SOLENOID;
    MotorMode motorMode = COOL;
    
    long triggerDelay = 0;
    long motorShutOffDelay = 0;
    int firingRate = 5;
    int burst1Amount = 3;
    int burst2Amount = 5;
    int fullAutoMaxAmount = 30;
    TriggerBehavior semiAutoTriggerBehavior = PASSIVE;
    TriggerBehavior burstfireTriggerBehavior = PASSIVE;
    bool isRevTriggerEnabled = false;
    bool rageOnRev = false;
    DigitalPin limiterPin;
    long pusherLimiterMaxTime = 500;
    int firingShotCycle = 50;
    

    int firingCycleStart = 0;
    int firingCount = 0;
    bool isFiringCycle = false;
    bool isFiring = false;
    long extraFiringCycleDelay = 0;
    long motorShutoffTime = 0;
    bool isMotorShutoff = false;
    long lastRevPressTime = 0;

    cbfunc_a pusherCallback;
    cbfunc_b motorCallback;
    bool isPusherCallbackAvailable = false;
    bool isMotorCallbackAvailable = false;
};


#endif