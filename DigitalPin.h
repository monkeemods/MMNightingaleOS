/*
  Digital Pin - Pin input handling library
  Created by sasaug
*/
#ifndef DigitalPin_h
#define DigitalPin_h

#include "Arduino.h"

class DigitalPin{
  public:
	  DigitalPin();
    #ifdef ESP32
      void setup(int number,int mode);
      void setupAndWrite(int number, int mode, int value);
    #else     
      void setup(int number, PinMode mode);
      void setupAndWrite(int number, PinMode mode, int value);
    #endif
    void enableInputDebounce(long value);
    bool isInputChanged();
    bool isOutputChanged(int value);
    bool read();
    void write(int value);

  private:
    bool poll();

    int number;
    #ifdef ESP32
      int mode;
    #else     
      PinMode mode;
    #endif
    bool state;
    bool lastState;
    long lastDebounceTime = millis();
    long debounceValue = 0;
};


#endif