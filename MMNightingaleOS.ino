/*
  Monkee Mods Nightingale OS
  Created by sasaug
*/

#include "MMNetwork.h"
#include "MMConfig.h"
#include "MMStats.h"
#include "MMBlaster.h"
#include "DigitalPin.h"
#ifdef ESP32
#include <ESP32Servo.h>
#else     
#include <Servo.h>
#endif

#define NAME "Blaster_Nightingale"
#define OS_VERSION "0.1.0"
#define BLASTER_TYPE "flywheel"

#ifdef ESP32
  #define MOTORESC_PIN D0
  #define PUSHERESC_PIN D1
  #define TRIGGERSWITCH_PIN D7
  #define REVSWITCH_PIN D8
  #define PROFILESWITCH_PIN D9
#else     
  #define MOTORESC_PIN 18
  #define PUSHERESC_PIN 19
  #define TRIGGERSWITCH_PIN 6
  #define REVSWITCH_PIN 7
  #define PROFILESWITCH_PIN 8
#endif

//CONSTANT VARIABLES
const String NETMSG_SERVER_TRIGGER_PRESS = "server;trigger_press";
const String NETMSG_SERVER_REV_MOTOR_CRUISE = "server;rev_motor_cruise";
const String NETMSG_SERVER_REV_MOTOR_FULL = "server;rev_motor_full";

//DYNAMIC VARIABLES
DigitalPin triggerSwitchPin;
DigitalPin revSwitchPin;
DigitalPin profileSwitchPin;
DigitalPin motorESCPin;
DigitalPin pusherESCPin;

int SPEEDMODE_FULL = 100; 
int SPEEDMODE_CRUISE = 22;
int SPEEDMODE_STOP = 0;
int PUSHER_FULL = 100;
int PUSHER_STOP = 100;

Servo pusherESC;
Servo motorESC;

MMBlaster blaster = MMBlaster(BRUSHED_FLYWHEEL);
MMNetwork network = MMNetwork(NAME);
MMConfig config = MMConfig(NAME, OS_VERSION, BLASTER_TYPE, network);
MMStats stats = MMStats();

bool emulateTriggerPress = false;

//Game Mode stuff
const String NETMSG_SERVER_GAMEMODE_ENABLE = "server;gamemode_enable";
const String NETMSG_SERVER_GAMEMODE_DISABLE = "server;gamemode_disable";
const String NETMSG_SERVER_GAMEMODE_MOTOR_CRUISE_SPEED = "server;gamemode_motor_cruise_speed;";
const String NETMSG_SERVER_GAMEMODE_MOTOR_FULL_SPEED = "server;gamemode_motor_full_speed;";
const String NETMSG_SERVER_GAMEMODE_TRIGGER_ENABLE = "server;gamemode_trigger_enable";
const String NETMSG_SERVER_GAMEMODE_TRIGGER_DISABLE = "server;gamemode_trigger_disable";
const String NETMSG_SERVER_GAMEMODE_FIRING_MODE = "server;gamemode_firing_mode;";
const String NETMSG_SERVER_GAMEMODE_MOTOR_MODE = "server;gamemode_motor_mode;";
const String NETMSG_SERVER_GAMEMODE_BURSTFIRE_AMOUNT = "server;gamemode_burstfire_amount;";

bool isGameMode = false;
bool gameModeTriggerEnabled = false;
String gameModeFiringMode = "semi";
String gameModeMotorMode = "rage";
int gameModeBurstfireAmount = 3;

#ifdef ESP32
  TaskHandle_t taskLoop1;
#endif

// Initialise and setting up the board on main thread
void setup() {
  //initialised the pin mode
  motorESCPin.setup(MOTORESC_PIN, OUTPUT);
  pusherESCPin.setup(PUSHERESC_PIN, OUTPUT);

  triggerSwitchPin.setup(TRIGGERSWITCH_PIN, INPUT_PULLUP);
  revSwitchPin.setup(REVSWITCH_PIN, INPUT_PULLUP);
  profileSwitchPin.setup(PROFILESWITCH_PIN, INPUT_PULLUP);

  motorESC.attach(MOTORESC_PIN);
  pusherESC.attach(PUSHERESC_PIN);
  armMotor();

  //initialise the serial
  Serial.begin(115200); 

  //setup stats
  stats.setup();

  //initialise config first
  config.init();

  //reset config when trigger is down on boot
  if(triggerSwitchPin.read()){
    config.clearConfig();
  }

  if(!config.haveConfig()){
    //base config
    config.setConfigString(CONFIG_KEY_BLASTERNAME, NAME, false, "Your Blaster Name");
    config.setConfigString(CONFIG_KEY_APWIFIPASSWORD, "monkeemods", false, "AP WiFi Password, secure your blaster's WiFi config page");
    config.setConfigString(CONFIG_KEY_LOGINUSERNAME, "", false, "Username for MMNetwork");
    config.setConfigString(CONFIG_KEY_LOGINPASSWORD, "", false, "Password for MMNetwork");
    config.setConfigString(CONFIG_KEY_WIFISSID, "", true, "WiFi SSID (multiple value supported, split with comma)");
    config.setConfigString(CONFIG_KEY_WIFIPASSWORD, "", true, "WiFi Password (multiple value supported, split with comma)");

    //blaster specific config
    config.setConfigString("profile1", "semi", false, "safe,semi,burst1,burst2,full", "Profile 1 ");
    config.setConfigString("profile2", "burst1", false, "safe,semi,burst1,burst2,full", "Profile 2");
  
    config.setConfigBool("rageOnRev", true, "Use rage mode when rev, else it will rev to full speed");
    config.setConfigInt("motorSpeedModeCruise", 21, "Cruise/Hot/Rage mode motor speed");
    config.setConfigInt("motorSpeedModeFull", 100, "Motor speed at full (when shots firing)");
    config.setConfigInt("pusherSpeedModeFull", 100, "Pusher speed");
    config.setConfigInt("triggerDelay", 0, "Motor spin up delay before firing");
    config.setConfigInt("motorShutoffDelay", 0, "Motor shutdown delay after done firing");
    config.setConfigInt("firingShotCycle", 50, "Time per shot cycle");
    config.setConfigInt("burstFireAmount1", 3, "Amount per burst fire shot");
    config.setConfigInt("burstFireAmount2", 5, "Amount per burst fire shot");
    config.setConfigInt("fullAutoMaxAmount", 30, "You can limit shots per full auto trigger pull");
    
    config.setConfigString("semiAutoBehavior", "reactive", false, "passive,reactive", "Semi auto trigger behavior");
    config.setConfigString("burstFireBehavior", "reactive", false, "passive,reactive", "Burst fire trigger behavior");
    config.saveConfig();
  }

  //setup config since settings is finalised now
  config.setup();

  //register callback
  network.setServerConnectedCallback(onServerConnected);
  network.setServerMessageCallback(onServerMessage);
  network.setup();

  //setup variable
  SPEEDMODE_CRUISE = config.getConfigInt("motorSpeedModeCruise");
  SPEEDMODE_FULL = config.getConfigInt("motorSpeedModeFull");
  PUSHER_FULL = config.getConfigInt("pusherSpeedModeFull");

  //setup blaster
  refreshBlasterMode();
  blaster.setMotorMode(COOL);
  blaster.setMotorShutoffDelay(config.getConfigInt("motorShutoffDelay"));
  blaster.setFiringShotCycle(config.getConfigInt("firingShotCycle"));
  blaster.setRageOnRev(config.getConfigBool("rageOnRev"));

  blaster.setBurst1Amount(config.getConfigInt("burstFireAmount1"));
  blaster.setBurst2Amount(config.getConfigInt("burstFireAmount2"));
  blaster.setFullAutoMaxAmount(config.getConfigInt("fullAutoMaxAmount"));
  blaster.setSemiAutoTriggerBehavior(config.getConfigString("semiAutoBehavior") == "passive" ? PASSIVE: REACTIVE);
  blaster.setBurstfireTriggerBehavior(config.getConfigString("burstFireBehavior") == "passive" ? PASSIVE: REACTIVE);
  blaster.setPusherCallback(onPusherUpdate);
  blaster.setMotorCallback(onMotorUpdate);
  blaster.setPusherType(TIMING_MOTOR);
  blaster.setRevTriggerEnabled(true);

  #ifdef ESP32
    xTaskCreatePinnedToCore(
      esp32Loop1,          
      "esp32Loop1",  
      10000,           
      NULL,           
      0,              
      &taskLoop1,        
      1);
  #endif
}

#ifdef ESP32
  void esp32Loop1(void * parameter){
    while(true){
      loop1();
    }
  } 
#endif

// Board's main loop
void loop() {
  //override the loop method if it's game mode
  if(isGameMode){
    loopGameMode();
    return;
  }

  //update firing mode if profile or fireselector switch change
  if(profileSwitchPin.isInputChanged()){
    refreshBlasterMode();
  }

  if(triggerSwitchPin.isInputChanged() || emulateTriggerPress){
    if(triggerSwitchPin.read() || emulateTriggerPress){
      blaster.triggerPress();
    }else{
      blaster.triggerRelease();
    }
  }

  if(revSwitchPin.isInputChanged()){
    if(!revSwitchPin.read()){
      blaster.revPress();
    }else{
      blaster.revRelease();
    }
  }

  blaster.loop();

  emulateTriggerPress = false;
  delay(1);                   
}

void refreshBlasterMode(){
  int profile = profileSwitchPin.read() ? 1: 2; 
  String key = "profile" + String(profile);
  String firingMode = config.getConfigString(key);
  if(firingMode == "safe"){
    blaster.setFiringMode(SAFE);
  }else if(firingMode == "semi"){
    blaster.setFiringMode(SEMI);
  }else if(firingMode == "burst1"){
    blaster.setFiringMode(BURST1);
  }else if(firingMode == "burst2"){
    blaster.setFiringMode(BURST2);
  }else if(firingMode == "full"){
    blaster.setFiringMode(FULLAUTO);
  }
}

//Game mode loop
void loopGameMode(){
  if(triggerSwitchPin.isInputChanged()){
    if(triggerSwitchPin.read()){
      blaster.triggerPress();
    }else{
      blaster.triggerRelease();
    }
  }

  if(revSwitchPin.isInputChanged()){
    if(revSwitchPin.read()){
      blaster.revPress();
    }else{
      blaster.revRelease();
    }
  }

  blaster.loop();
  delay(10);    
}

// Board's second loop for networking purposes
void loop1(){
  network.loop();
  config.loop();
  stats.loop();
  delay(100);
}

void setMotorSpeed(int speedMode) {
  motorESC.writeMicroseconds(map(speedMode, 0, 100, 1000, 2000));
}

void setPusherSpeed(int speedMode) {
  pusherESC.writeMicroseconds(map(speedMode, 0, 100, 1000, 2000));
}

void armMotor(){
  //arm speed controller, modify as necessary for your ESC
  setMotorSpeed(SPEEDMODE_STOP);
  setPusherSpeed(PUSHER_STOP);
  delay(1000);
  setMotorSpeed(SPEEDMODE_FULL);
  setPusherSpeed(PUSHER_FULL);
  delay(1000);
  setMotorSpeed(SPEEDMODE_STOP);
  setPusherSpeed(PUSHER_STOP);
  Serial.println("Armed motor");
  delay(1000);
}

void onServerConnected(){
}

void onServerMessage(String msg){
  config.onServerMessage(msg);
  if(msg == NETMSG_SERVER_GAMEMODE_ENABLE){
    isGameMode = true;
  }else if(msg == NETMSG_SERVER_GAMEMODE_DISABLE){
    isGameMode = false;
    delay(3000);
    #ifdef ESP32
      ESP.restart();
    #else
      rp2040.reboot();
    #endif
  }else if(msg == NETMSG_SERVER_TRIGGER_PRESS){
    emulateTriggerPress = true;
  }else if(msg == NETMSG_SERVER_REV_MOTOR_CRUISE){
    setMotorSpeed(SPEEDMODE_CRUISE);
    delay(3000);
    setMotorSpeed(SPEEDMODE_STOP);
  }else if(msg == NETMSG_SERVER_REV_MOTOR_FULL){
    setMotorSpeed(SPEEDMODE_FULL);
    delay(3000);
    setMotorSpeed(SPEEDMODE_STOP);
  }else if(msg.indexOf(NETMSG_SERVER_GAMEMODE_MOTOR_CRUISE_SPEED) > -1 && isGameMode){
    msg.replace(NETMSG_SERVER_GAMEMODE_MOTOR_CRUISE_SPEED, "");
    SPEEDMODE_CRUISE = msg.toInt();
  }else if(msg.indexOf(NETMSG_SERVER_GAMEMODE_MOTOR_FULL_SPEED) > -1 && isGameMode){
    msg.replace(NETMSG_SERVER_GAMEMODE_MOTOR_FULL_SPEED, "");
    SPEEDMODE_FULL = msg.toInt();
  }else if(msg == NETMSG_SERVER_GAMEMODE_TRIGGER_ENABLE && isGameMode){
    gameModeTriggerEnabled = true;
  }else if(msg == NETMSG_SERVER_GAMEMODE_TRIGGER_DISABLE && isGameMode){
    gameModeTriggerEnabled = false;
  }else if(msg.indexOf(NETMSG_SERVER_GAMEMODE_FIRING_MODE) > -1 && isGameMode){
    msg.replace(NETMSG_SERVER_GAMEMODE_FIRING_MODE, "");
    gameModeFiringMode = msg;
  }else if(msg.indexOf(NETMSG_SERVER_GAMEMODE_MOTOR_MODE) > -1 && isGameMode){
    msg.replace(NETMSG_SERVER_GAMEMODE_MOTOR_MODE, "");
    gameModeMotorMode = msg;
  }else if(msg.indexOf(NETMSG_SERVER_GAMEMODE_BURSTFIRE_AMOUNT) > -1 && isGameMode){
    msg.replace(NETMSG_SERVER_GAMEMODE_BURSTFIRE_AMOUNT, "");
    gameModeBurstfireAmount = msg.toInt();
  }
}

void onPusherUpdate(MotorSpeed speed, bool isFired){
  if(speed == STOP){
    setPusherSpeed(PUSHER_STOP);
  }else{
    setPusherSpeed(PUSHER_FULL);
  }
}

void onMotorUpdate(MotorSpeed speed){
  if(speed == STOP){
    setMotorSpeed(SPEEDMODE_STOP);
  }else if(speed == CRUISE){
    setMotorSpeed(SPEEDMODE_CRUISE);
  }else{
    setMotorSpeed(SPEEDMODE_FULL);
  }
}
//Stats related stuff
void statsAddShotsFired(){
  stats.setStatsInt("shotsFired", stats.getStatsInt("shotsFired")+1);
}

void statsAddSemiFired(){
  stats.setStatsInt("shotsFiredSemi", stats.getStatsInt("shotsFiredSemi")+1);
}

void statsAddBurstFired(){
  stats.setStatsInt("shotsFiredBurst", stats.getStatsInt("shotsFiredBurst")+1);
}

void statsAddFullAutoFired(){
  stats.setStatsInt("shotsFiredFullAuto", stats.getStatsInt("shotsFiredFullAuto")+1);
}



