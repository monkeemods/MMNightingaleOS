#include "MMBlaster.h"
#include "Arduino.h"
#include "DigitalPin.h"  

MMBlaster::MMBlaster(BlasterType type){
  blasterType = type;
}

void MMBlaster::loop(){
  //check if motor need to spin down
  if((blasterType == FLYWHEEL || blasterType == BRUSHED_FLYWHEEL) && !isFiring && !isMotorShutoff){
    if(isRevTriggerEnabled && lastRevPressTime != 0){
      isMotorShutoff = true;
      callbackMotor(rageOnRev? CRUISE: MAX);
    }else if(millis() >= motorShutoffTime){
      if((isRevTriggerEnabled && rageOnRev && lastRevPressTime != 0) || (firingMode != SAFE && motorMode == RAGE)){
        callbackMotor(CRUISE);
      }else{
        callbackMotor(STOP);
      } 
      isMotorShutoff = true;  
    }
  }

  //if still got darts to fire
  if(firingCount > 0){
    if(pusherType == TIMING_SOLENOID){
      long firingCycleDelay = 1000/2/firingRate;
      if(!isFiring){
        isFiring = true;
        if(blasterType == FLYWHEEL || blasterType == BRUSHED_FLYWHEEL){
          callbackMotor(MAX);
        }
        long delay = 0;
        if(isRevTriggerEnabled && lastRevPressTime != 0){
          delay = triggerDelay - (millis() - lastRevPressTime);
          if(delay <= 0){
            delay = 0;
          }
        }else{
          delay = triggerDelay;
        }        
        firingCycleStart =  millis() - firingCycleDelay + delay;
      }else if(millis() >= firingCycleStart + firingCycleDelay){
        if(isFiringCycle){
          firingCount--;
          callbackPusher(STOP, true);
          firingCycleStart = millis();
          isFiringCycle = false;
        }else{
          isFiringCycle = true;
          callbackPusher(MAX, false);
          firingCycleStart = millis();
        }
      }
    }else if(pusherType == TIMING_MOTOR){
      long firingCycleDelay = firingShotCycle;
      if(!isFiring){
        isFiring = true;
        if(blasterType == FLYWHEEL || blasterType == BRUSHED_FLYWHEEL){
          callbackMotor(MAX);
        }  

        long delay = 0;
        if(isRevTriggerEnabled && lastRevPressTime != 0){
          delay = triggerDelay - (millis() - lastRevPressTime);
          if(delay <= 0){
            delay = 0;
          }
        }else{
          delay = triggerDelay;
        } 

        firingCycleStart =  millis() - firingCycleDelay + delay;
      }else if(millis() >= firingCycleStart + firingCycleDelay){
        if(isFiringCycle){
          firingCount--;
          callbackPusher(MAX, true);
          firingCycleStart = millis();
        }else{
          isFiringCycle = true;
          callbackPusher(MAX, false);
          firingCycleStart = millis();
        }
      }
    }else{
      if(!isFiring){
        isFiring = true;   
        long delay = 0;
        if(isRevTriggerEnabled && lastRevPressTime != 0){
          delay = triggerDelay - (millis() - lastRevPressTime);
          if(delay <= 0){
            delay = 0;
          }
        }else{
          delay = triggerDelay;
        } 
        firingCycleStart =  millis() + delay;
      }else if(millis() >= firingCycleStart || (limiterPin.isInputChanged() && limiterPin.read())){
        if(isFiringCycle){
          firingCount--;
          callbackPusher(STOP, true);
          firingCycleStart = millis() + extraFiringCycleDelay;
          isFiringCycle = false;
        }else{
          isFiringCycle = true;
          callbackPusher(MAX, false);
          firingCycleStart = millis() + pusherLimiterMaxTime;
        }
      }
    }    
  }else{
    if(isFiring){
      isFiring = false;
      isFiringCycle = false;
      callbackPusher(STOP, false);
      motorShutoffTime = millis() + motorShutOffDelay;
      isMotorShutoff = false;
    }
  }
}

void MMBlaster::triggerPress(){
  if(firingMode == SEMI){
    if(semiAutoTriggerBehavior == REACTIVE){
      firingCount++;
    }else{
      if(!isFiring){
        firingCount = 1;
      }
    }
  }else if(firingMode == BURST1){
    if(burstfireTriggerBehavior == REACTIVE){
      firingCount += burst1Amount;
    }else{
      if(!isFiring){
        firingCount = burst1Amount;
      }
    }
  }else if(firingMode == BURST2){
    if(burstfireTriggerBehavior == REACTIVE){
      firingCount += burst2Amount;
    }else{
      if(!isFiring){
        firingCount = burst2Amount;
      }
    }
  }else if(firingMode == FULLAUTO){
    if(!isFiring){
      firingCount = fullAutoMaxAmount;
    }
  }
}

void MMBlaster::triggerRelease(){
  if(firingMode == FULLAUTO){
    firingCount = 0;
  }
}

void MMBlaster::revPress(){
  if(!isFiring){
    lastRevPressTime = millis();
    callbackMotor(rageOnRev? CRUISE: MAX);
  }
}

void MMBlaster::revRelease(){
  lastRevPressTime = 0;
  if(!isFiring){
    callbackMotor(STOP);
  }
}

void MMBlaster::setFiringMode(FiringMode mode){
  if(firingMode != mode){
    firingMode = mode;
  }
}

void MMBlaster::setMotorMode(MotorMode mode){
  if(motorMode != mode){
    motorMode = mode;
  }
}

void MMBlaster::setTriggerDelay(long delay){
  triggerDelay = delay;
}

void MMBlaster::setMotorShutoffDelay(long delay){
  motorShutOffDelay = delay;
}

void MMBlaster::setExtraFiringCycleDelay(long delay){
  extraFiringCycleDelay = delay;
}

void MMBlaster::setFiringRate(int count){
  firingRate = count;
}

void MMBlaster::setBurst1Amount(int value){
  burst1Amount = value;
}

void MMBlaster::setBurst2Amount(int value){
  burst2Amount = value;
}

void MMBlaster::setFullAutoMaxAmount(int value){
  fullAutoMaxAmount = value;
}

void MMBlaster::setSemiAutoTriggerBehavior(TriggerBehavior behavior){
  semiAutoTriggerBehavior = behavior;
}

void MMBlaster::setBurstfireTriggerBehavior(TriggerBehavior behavior){
  burstfireTriggerBehavior = behavior;
}

void MMBlaster::setPusherCallback(cbfunc_a callbackFunction) {
    pusherCallback = callbackFunction;
    isPusherCallbackAvailable = true;
}

void MMBlaster::setMotorCallback(cbfunc_b callbackFunction) {
    motorCallback = callbackFunction;
    isMotorCallbackAvailable = true;
}

void MMBlaster::callbackMotor(MotorSpeed speed){
  if(isMotorCallbackAvailable){
    motorCallback(speed);
  }
}

void MMBlaster::callbackPusher(MotorSpeed speed, bool isFired){
  if(isPusherCallbackAvailable){
    pusherCallback(speed, isFired);
  }
}

void MMBlaster::setPusherType(PusherType type){
  pusherType = type;
}

void MMBlaster::attachPusherLimiterPin(DigitalPin pin){
  limiterPin = pin;
}

FiringMode MMBlaster::getFiringMode(){
  return firingMode;
}

MotorMode MMBlaster::getMotorMode(){
  return motorMode;
}

bool MMBlaster::isBlasterFiring(){
  return this->isFiring;
}

void MMBlaster::setRevTriggerEnabled(bool enable){
  isRevTriggerEnabled = enable;
}

void MMBlaster::setRageOnRev(bool enable){
  rageOnRev = enable;
}

void MMBlaster::setFiringShotCycle(int shotCycle){
  firingShotCycle = shotCycle;
}

