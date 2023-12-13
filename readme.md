# MM Nightingale OS
This is a "smarter" Nightingale, powered by Pico W(RP2040) or ESP32, with the aim to maintain most of it's hardware, but allow it to be more configurable/programmable.
It uses the same library as our other projects, similar to the library that powered our reverse engineered Diana.

## Concept
The ON/OFF button at the back have been set as Profile 1 and Profile 2. The pusher is based on timing so adjust based on how your semi auto perform.
The firing cycle is powered by MMBlaster library so you can set the trigger delay or even enable/disable rage mode(not implemented for now as this is dependant of the motor ESC you use)
This is an implementation of MMBlaster, to be used in brushed flywheel blaster.

## Configuration
- blasterName
Set a name for your blaster. This is also used as the blaster's WiFi SSID. Default to `Blaster_Nightingale`

- apWiFiPassword
Password for your blaster's WiFi password. Default to `monkeemods`

- loginUsername & loginPassword
For future use when connect to MMNetwork

- wifiSSID
Connect to a WiFi router SSID (eg your home network). You can set a few SSID, seperate them with ,
Eg: network1,network2,network3

- wifiPassword
The password for WiFi SSID.
Eg: network1password,network2password,network3password

- profile1/profile2
You can use `safe`,`semi`,`burst1`,`burst2`,`full`

Safe = Safety

Semi = 1 shot

Burst1 = Shots configured in `burstFireAmount1`

Burst2 = Shots configured in `burstFireAmount2`

Auto = Full auto with maximum limit set in `fullAutoMaxAmount`

- triggerDelay
Delay after pulling the trigger to firing to ensure flywheel spin up to speed. Value is milliseconds

- motorShutoffDelay
The delay to turn off the flywheel after finish firing. Value is milliseconds

- firingRate
Timing per shot cycle. Adjust this lower if your semi is firing more than 1 shot. Value is milliseconds.

- burstFireAmount1
The amount of burstfire shot for `burst1`

- burstFireAmount2
The amount of burstfire shot for `burst2`

- fullAutoMaxAmount
The maximum shot for full auto. You can limit this to control your full auto or do 1-X amount of shots, based on the value you set.

- semiAutoBehavior/burstFireBehavior

There are 2 types of trigger behavior, `passive` and `reactive`.

In `passive` mode, it will ignore any other trigger pull in mid firing cycle. Example for burstfire, while it fires off all 3 burst shots, if you pull your trigger again, your blaster will fire a total of 3 shots with 2 trigger pull in total.

In `reactive` mode, it WILL NOT ignore the trigge pull in mid firing cycle. While it still firing the first set of 3 shots, if you press the trigger again, a total of 6 shots will be fired.

This applies to semi auto as well but since the speed is too fast, you will barely notice this unless you do it on purpose. Using `passive` trigger might give you the sense of the trigger malfunction so configure this based on your preference.

## Hardware Needed
- Raspberry Pico W / ESP32 (I used Seeed Xiao ESP32 as it's super small and suit our use case)
- Brushed ESC for Motor (10-20A at least, or you can use MOSFET) (I'm using 20A and use it to power both motors)
- Brushed ESC for Pusher (few A will do as it's a very small motor, or you can use MOSFET) (I'm using 10A)
- BEC (unless your ESC have BEC) (My Motor ESC have BEC)

## Software Dependencies
- Arduino Pico (if you are using Pico)
- Arduino ESP32 and ESP32Servo (if you are using ESP32)
- MMBlaster, MMNetwork, MMConfig, MMStats (included)

## Installation
1. Wire the blaster as per the wiring diagram provided
2. To flash your microcontroller:
- Install Arduino IDE
- Install ESP32 Arduino library (if you using ESP32)/ Arduino-Pico (if you using Pico W)
- Download the source code and open it in Arduino IDE
- Install ESP32Servo library in ArduinoIDE(if you are using ESP32)
- Install ArduinoJson library in ArduinoIDE
- Connect your microcontroller in download/flashing mode(usually holding the B button while connecting the cable)
- Click the Upload button in ArduinoIDE
3. If you are updating to a new version, check if there are change of parameters, if yes, you have to HOLD your trigger when you connect battery to reset the config

## Note
This is built for ESP32(Seeed Xiao ESP32) but Pico W should be supported. Since Nightingale have lesser space, hence the Xiao ESP32 board was chosen.

