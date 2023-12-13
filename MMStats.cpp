#include "MMStats.h"
#include "Arduino.h"  
#include <LittleFS.h>  
#include <ArduinoJson.h>

MMStats::MMStats(){}

void MMStats::setup(){
  #ifdef ESP32
    LittleFS.begin(true);
  #else
    LittleFS.begin();
  #endif
  loadStats();
}

void MMStats::loop(){
  if(millis() >= lastSavedTime + STATS_SAVE_FREQUENCY){
    saveStats();
    lastSavedTime = millis();
  }
}

void MMStats::setStatsString(String name, String value){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valString = value;
      return;
    }
  }

  Stats c = Stats(name);
  c.setString(value);
  variables[variablesCount] = c;
  variablesCount++;
}

void MMStats::setStatsInt(String name, int value){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valInt = value;
      return;
    }
  }

  Stats c = Stats(name);
  c.setInt(value);
  variables[variablesCount] = c;
  variablesCount++;
}

void MMStats::setStatsLong(String name, long value){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valLong = value;
      return;
    }
  }

  Stats c = Stats(name);
  c.setLong(value);
  variables[variablesCount] = c;
  variablesCount++;
}

void MMStats::setStatsDouble(String name, double value){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valDouble = value;
      return;
    }
  }

  Stats c = Stats(name);
  c.setDouble(value);
  variables[variablesCount] = c;
  variablesCount++;
}

void MMStats::setStatsBool(String name, bool value){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valBool = value;
      return;
    }
  }

  Stats c = Stats(name);
  c.setBool(value);
  variables[variablesCount] = c;
  variablesCount++;
}

String MMStats::getStatsString(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return variables[i].valString;
    }
  }
  return "";
}

int MMStats::getStatsInt(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return variables[i].valInt;
    }
  }
  return 0;
}

long MMStats::getStatsLong(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return variables[i].valLong;
    }
  }
  return 0;
}

double MMStats::getStatsDouble(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return variables[i].valDouble;
    }
  }
  return 0;
}

bool MMStats::getStatsBool(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return variables[i].valBool;
    }
  }
  return false;
}

String MMStats::getStatsJson(){
  DynamicJsonDocument cfg = DynamicJsonDocument(MAX_STATS_JSON_SIZE);

  for(int i = 0; i < variablesCount; i++){
      Stats c = variables[i];
      cfg["variables"][i]["key"] = c.key;
      if(c.type == "string"){
        cfg["variables"][i]["value"] = c.valString;
        cfg["variables"][i]["type"] = c.type;
      }else if(c.type == "int"){
        cfg["variables"][i]["value"] = c.valInt;
        cfg["variables"][i]["type"] = c.type;
      }else if(c.type == "long"){
        cfg["variables"][i]["value"] = c.valLong;
        cfg["variables"][i]["type"] = c.type;
      }else if(c.type == "double"){
        cfg["variables"][i]["value"] = c.valDouble;
        cfg["variables"][i]["type"] = c.type;
      }else if(c.type == "bool"){
        cfg["variables"][i]["value"] = c.valBool;
        cfg["variables"][i]["type"] = c.type;
      }
  }
  String result;
  serializeJson(cfg, result);
  return result;
}

void MMStats::parseStatsJson(String json){
  DynamicJsonDocument cfg = DynamicJsonDocument(MAX_STATS_JSON_SIZE);
  deserializeJson(cfg, json);

  variablesCount = cfg["variables"].as<JsonArray>().size();

  for(int i = 0; i < variablesCount; i++){
    String key = cfg["variables"][i]["key"];
    String type = cfg["variables"][i]["type"];

    if(type == "string"){
      String value = cfg["variables"][i]["value"];
      Stats c = Stats(key);
      c.setString(value);
      variables[i] = c;
    }else if(type == "int"){
      int value = cfg["variables"][i]["value"];
      Stats c = Stats(key);
      c.setInt(value);
      variables[i] = c;
    }else if(type == "long"){
      long value = cfg["variables"][i]["value"];
      Stats c = Stats(key);
      c.setLong(value);
      variables[i] = c;
    }else if(type == "double"){
      double value = cfg["variables"][i]["value"];
      Stats c = Stats(key);
      c.setDouble(value);
      variables[i] = c;
    }else if(type == "bool"){
      bool value = cfg["variables"][i]["value"];
      Stats c = Stats(key);
      c.setBool(value);
      variables[i] = c;
    }
  }
}

bool MMStats::haveStats(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return true;
    }
  }
  return false;
}

void MMStats::loadStats(){
  File file = LittleFS.open(F(STATS_FILE_LOCATION), "r");
  if (file) {
    String str = file.readString();
    parseStatsJson(str);
    file.close();
  }
}

void MMStats::saveStats(){
  File file = LittleFS.open(F(STATS_FILE_LOCATION), "w");
  if (file) {
    file.println(getStatsJson());
    file.close();
  }
}

void MMStats::clearStats(){
  LittleFS.remove(F(STATS_FILE_LOCATION));
  variablesCount = 0;
}

int MMStats::convertStringToArray(char** list, String s, String seperator){
  s += ".";
  char* buffer = new char[s.length()];
  s.toCharArray(buffer, s.length());

  int count = 1;
  for(int i = 0; i < s.length();i++){
    if(buffer[i] == seperator.charAt(0)){
      count++;
    }
  }

  
  int index = 0;
  char* token = strtok(buffer, seperator.c_str());
  while (token != NULL) {
      list[index] = token;
      token = strtok(NULL, seperator.c_str());
      index++;
  }

  return count;
}