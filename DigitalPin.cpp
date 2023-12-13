#include "DigitalPin.h"
#include "Arduino.h"

DigitalPin::DigitalPin(){}

#ifdef ESP32
  void DigitalPin::setup(int number, int mode){
    this->number = number;
    this->mode = mode;

    pinMode(number, mode);

    if(mode == INPUT_PULLUP || mode == INPUT_PULLDOWN){
      this->state = poll();
    }else{
      this->state = 0;
    }
    
    this->lastState = this->state;
  }

  void DigitalPin::setupAndWrite(int number, int mode, int value){
    setup(number, mode);
    write(1);
  }
#else
  void DigitalPin::setup(int number, PinMode mode){
    this->number = number;
    this->mode = mode;

    pinMode(number, mode);

    if(mode == INPUT_PULLUP || mode == INPUT_PULLDOWN){
      this->state = poll();
    }else{
      this->state = 0;
    }
    
    this->lastState = this->state;
  }

  void DigitalPin::setupAndWrite(int number, PinMode mode, int value){
    setup(number, mode);
    write(1);
  }
#endif


void DigitalPin::enableInputDebounce(long value){
  this->debounceValue = value;
}

bool DigitalPin::isInputChanged(){
  bool val = poll();
  if(val != state){
    if(debounceValue == 0){
      state = val;
      return true;
    }

    if(lastState != val){
      lastDebounceTime = millis();
    }else if(millis() - lastDebounceTime >= debounceValue){
      state = val;
      return true;
    }
  }
  lastState = val;
  return false;
}

bool DigitalPin::isOutputChanged(int value){
  return lastState != value;
}

bool DigitalPin::read(){
  bool val = poll();
  if(val != state){
    if(debounceValue == 0){
      state = val;
      return state;
    }

    if(lastState != val){
      lastDebounceTime = millis();
    }else if(millis() - lastDebounceTime >= debounceValue){
      state = val;
      return state;
    }
  }
  lastState = val;
  return state;
}

bool DigitalPin::poll(){
  if(mode == INPUT_PULLUP){
    return digitalRead(number) != 1;
  }else{
    return digitalRead(number) == 1;
  }
}

void DigitalPin::write(int value){
  digitalWrite(number, value);
  lastState = value;
}
