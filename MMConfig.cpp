#include "MMConfig.h"
#include "Arduino.h"                                                                                                                             
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "MMNetwork.h"
#include <WebServer.h>

MMConfig::MMConfig(String name, String osVersion, String type, MMNetwork& mmNetwork){
	this->name = name;
  this->type = type;
  this->osVersion = osVersion;
  this->netVersion = mmNetwork.getVersion();
  this->mmNetwork = &mmNetwork;
}

void MMConfig::init(){
  #ifdef ESP32
    LittleFS.begin(true);
  #else
    LittleFS.begin();
  #endif
  loadConfig();
}

void MMConfig::setup(){
  this->name = getConfigString(CONFIG_KEY_BLASTERNAME);
  
  //add all the ssid from config
  char *ssidArray[10];
  int sizeSSIDArray = getConfigStringAsArray(ssidArray, CONFIG_KEY_WIFISSID);
  char *ssidPasswordArray[10];
  int sizeSSIDPasswordArray = getConfigStringAsArray(ssidPasswordArray, CONFIG_KEY_WIFIPASSWORD);

  int count = 0;
  for(int i = 0; i < sizeSSIDArray; i++){
    if(ssidArray[i] != ""){
        mmNetwork->addWiFiSSID(ssidArray[i], ssidPasswordArray[i]);
        count++;
    }
  }

  if(count == 0){
    setConfigMode(true);
    mmNetwork->setWiFiMode(WIFIMODE_AP);
    mmNetwork->setWiFiAPSSID(name);
    mmNetwork->setWiFiAPPassword(getConfigString(CONFIG_KEY_APWIFIPASSWORD));
  }else{
    setConfigMode(false);
  }

  mmNetwork->setLoginUsername(getConfigString(CONFIG_KEY_LOGINUSERNAME));
  mmNetwork->setLoginPassword(getConfigString(CONFIG_KEY_LOGINPASSWORD));

  server = new WebServer(80);
}

void MMConfig::loop(){
  if(mmNetwork->isWiFiConnected()){
    if(!_isConfigServerStarted){
      if (MDNS.begin("blaster")) {
        Serial.println("MDNS responder started");
      }

      server->on("/", [&]() {
          server->send(200, "text/html", getHtml());
      });
      server->on("/main.css", [&](){
          server->send(200, "text/css", MAIN_CSS);
      });
      server->on("/config", [&](){
          if (server->method() == HTTP_POST) {
            String message = "Configuration:<br/>";
            bool hidePassword = false;
            for (uint8_t i = 0; i < server->args(); i++) {
                if(server->argName(i) == "_hidePassword"){
                  hidePassword = server->arg(i) == "on";
                }
            }

            for (uint8_t i = 0; i < server->args(); i++) {
              if(hidePassword && server->argName(i).charAt(0) != '_' && (server->argName(i).indexOf("password") > -1 || server->argName(i).indexOf("Password") > -1)){
                message += server->argName(i) + ": ";
                for(int j = 0; j < server->arg(i).length(); j++){
                  message +="*";
                }
                message += "<br>";
              }else{
                message += server->argName(i) + ": " + server->arg(i) + "<br>";
              }

              for(int j = 0; j < variablesCount; j++){
                  Config c = variables[j];
                  if(c.key == server->argName(i)){
                    if(c.type == "string"){
                      variables[j].valString = server->arg(i);
                    }else if(c.type == "int"){
                      variables[j].valInt = server->arg(i).toInt();
                    }else if(c.type == "long"){
                      variables[j].valLong = server->arg(i).toInt();
                    }else if(c.type == "double"){
                      variables[j].valDouble = server->arg(i).toDouble();
                    }else if(c.type == "bool"){
                      variables[j].valBool = (server->arg(i) == "1");
                    }
                  }
              }
            }
            saveConfig();
            message += "<br>Configuration saved. Blaster will now reboot.....";
            server->send(200, "text/html", message);
            delay(5000);
            #ifdef ESP32
              ESP.restart();
            #else
              rp2040.reboot();
            #endif
          }
      });

      server->begin();
      #ifdef ESP32
        MDNS.addService("http", "tcp", 80);
      #endif
      Serial.println("HTTP server started");
      _isConfigServerStarted = true;
    }else{
      server->handleClient();
    }
  }
}

void MMConfig::setConfigString(String name, String value, bool isArray, String description){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valString = value;
      variables[i].isValStringArray = isArray;
      return;
    }
  }

  Config c = Config(name, value, isArray, description);
  variables[variablesCount] = c;
  variablesCount++;
}

void MMConfig::setConfigString(String name, String value, bool isArray, String range, String description){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valString = value;
      variables[i].isValStringArray = isArray;
      variables[i].valStringRange = range;
      return;
    }
  }

  Config c = Config(name, value, isArray, range, description);
  variables[variablesCount] = c;
  variablesCount++;
}

void MMConfig::setConfigInt(String name, int value, String description){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valInt = value;
      return;
    }
  }
  Config c = Config(name, description);
  c.setInt(value);
  variables[variablesCount] = c;
  variablesCount++;
}

void MMConfig::setConfigLong(String name, long value, String description){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valLong = value;
      return;
    }
  }
  Config c = Config(name, description);
  c.setLong(value);
  variables[variablesCount] = c;
  variablesCount++;
}

void MMConfig::setConfigDouble(String name, double value, String description){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valDouble = value;
      return;
    }
  }

  Config c = Config(name, description);
  c.setDouble(value);
  variables[variablesCount] = c;
  variablesCount++;
}

void MMConfig::setConfigBool(String name, bool value, String description){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      variables[i].valBool = value;
      return;
    }
  }
  Config c = Config(name, description);
  c.setBool(value);
  variables[variablesCount] = c;
  variablesCount++;
}

String MMConfig::getConfigString(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return variables[i].valString;
    }
  }
  return "";
}

int MMConfig::getConfigStringAsArray(char** list, String name){
  String s = getConfigString(name);
  if(s == ""){
    return 0;
  }
  return convertStringToArray(list, s, ",");
}

int MMConfig::getConfigInt(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return variables[i].valInt;
    }
  }
  return -1;
}

long MMConfig::getConfigLong(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return variables[i].valLong;
    }
  }
  return -1;
}

double MMConfig::getConfigDouble(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return variables[i].valDouble;
    }
  }
  return -1;
}

bool MMConfig::getConfigBool(String name){
  for(int i = 0; i < variablesCount; i++){
    if(variables[i].key == name){
      return variables[i].valBool;
    }
  }
  return false;
}

String MMConfig::getConfigJson(bool includeInfo){
  DynamicJsonDocument cfg = DynamicJsonDocument(MAX_CONFIG_JSON_SIZE);
  if(includeInfo){
    cfg["name"] = name;
    cfg["type"] = type;
    #ifdef ESP32
      cfg["board"] = "ESP32";
    #else
      cfg["board"] = BOARD_NAME;
    #endif
    cfg["netVersion"] = netVersion;
  }

  for(int i = 0; i < variablesCount; i++){
      Config c = variables[i];
      cfg["variables"][i]["key"] = c.key;
      cfg["variables"][i]["description"] = c.description;
      if(c.type == "string"){
        cfg["variables"][i]["value"] = c.valString;
        cfg["variables"][i]["type"] = c.type;
        cfg["variables"][i]["isArray"] = c.isValStringArray;
        cfg["variables"][i]["range"] = c.valStringRange;
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

void MMConfig::parseConfigJson(String json){
  DynamicJsonDocument cfg = DynamicJsonDocument(MAX_CONFIG_JSON_SIZE);
  deserializeJson(cfg, json);

  variablesCount = cfg["variables"].as<JsonArray>().size();

  for(int i = 0; i < variablesCount; i++){
    String key = cfg["variables"][i]["key"];
    String type = cfg["variables"][i]["type"];
    String description = cfg["variables"][i]["description"];

    if(type == "string"){
      bool isArray = cfg["variables"][i]["isArray"];
      String value = cfg["variables"][i]["value"];
      String range = cfg["variables"][i]["range"];
      Config c = Config(key, value, isArray, range, description);
      variables[i] = c;
    }else if(type == "int"){
      int value = cfg["variables"][i]["value"];
      Config c = Config(key, description);
      c.setInt(value);
      variables[i] = c;
    }else if(type == "long"){
      long value = cfg["variables"][i]["value"];
      Config c = Config(key, description);
      c.setLong(value);
      variables[i] = c;
    }else if(type == "double"){
      double value = cfg["variables"][i]["value"];
      Config c = Config(key, description);
      c.setDouble(value);
      variables[i] = c;
    }else if(type == "bool"){
      bool value = cfg["variables"][i]["value"];
      Config c = Config(key, description);
      c.setBool(value);
      variables[i] = c;
    }
  }
}

bool MMConfig::haveConfig(){
  return LittleFS.exists(F(CONFIG_FILE_LOCATION));
}

void MMConfig::loadConfig(){
  File file = LittleFS.open(F(CONFIG_FILE_LOCATION), "r");
  if (file) {
    String str = file.readString();
    parseConfigJson(str);
    file.close();
  }
}

void MMConfig::saveConfig(){
  File file = LittleFS.open(F(CONFIG_FILE_LOCATION), "w");
  if (file) {
    file.println(getConfigJson(false));
    file.close();
  }
}

void MMConfig:: clearConfig(){
  LittleFS.remove(F(CONFIG_FILE_LOCATION));
  variablesCount = 0;
}

void MMConfig::setConfigMode(bool enabled){
  this->_isConfigMode = enabled;
}

bool MMConfig::isConfigMode(){
  return this->_isConfigMode;
}

int MMConfig::convertStringToArray(char** list, String s, String seperator){
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

String MMConfig::getHtml(){
  String title = name + " Configuration";
  String buffer = "<html><head><title>"+title+"</title><link rel=\"stylesheet\" href=\"/main.css\"></head><body style=\"padding:1em\"><form class=\"pure-form pure-form-stacked\" method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/config\"><fieldset>";
  for(int i = 0; i < variablesCount; i++){
      Config c = variables[i];
      if(c.type == "string"){
        if(c.key.indexOf("password") > -1 || c.key.indexOf("Password") > -1){
          buffer += "<label>"+c.key+" (" + c.description + "):</label><input type=\"password\" name=\""+c.key+"\" value=\""+c.valString+"\" class=\"pure-input-1-2\">";
        }else if(c.valStringRange != ""){
          buffer += "<div class=\"pure-g\"><label>"+c.key+" (" + c.description + "):</label>";
          buffer += "<select name=\""+c.key+"\">";
          char *array[10];
          int size = convertStringToArray(array, c.valStringRange, ",");
          for(int i = 0; i < size; i++){     
            String s = String(array[i]);
            if(s == c.valString){
              buffer += "<option value=\""+s+"\" selected>"+s+"</option>";
            }else{
              buffer += "<option value=\""+s+"\">"+s+"</option>";
            }
          }
          buffer += "</select></div>";
        }else{
          buffer += "<label>"+c.key+" (" + c.description + "):</label><input type=\"text\" name=\""+c.key+"\" value=\""+c.valString+"\" class=\"pure-input-1-2\">";
        }
      }else if(c.type == "int"){
        buffer += "<label>"+c.key+" (" + c.description + "):</label><input type=\"text\" name=\""+c.key+"\" value=\""+c.valInt+"\">";
      }else if(c.type == "long"){
        buffer += "<label>"+c.key+" (" + c.description + "):</label><input type=\"text\" name=\""+c.key+"\" value=\""+c.valLong+"\">";
      }else if(c.type == "double"){
        buffer += "<label>"+c.key+" (" + c.description + "):</label><input type=\"text\" name=\""+c.key+"\" value=\""+c.valDouble+"\"";
      }else if(c.type == "bool"){
        String checked = c.valBool? "checked": "";
        String value = c.valBool? "1": "0";
        buffer += "<label class=\"pure-checkbox\"><input type=\"hidden\" id=\""+c.key+"\"name=\""+c.key+"\" value="+value +"><input type=\"checkbox\" onclick=\"this.previousSibling.value=1-this.previousSibling.value\" "+checked+"/>"+c.key+" (" + c.description + ")</label>";
      }
  }
  buffer += "<label class=\"pure-checkbox\"><input type=\"checkbox\" name=\"_hidePassword\" checked/>Hide Password on done page</label>";
  buffer += "<input type=\"submit\" class=\"pure-button pure-button-primary\" value=\"Apply Config\"></fieldset></form></body></html>";
  return buffer;
}

void MMConfig::sendConfigToServer(){
  //if server connected, we will submit our blaster
    if(mmNetwork->isServerConnected() && mmNetwork->getNetworkStatus() == STATUS_SERVER_CONNECTED){
      String message = NETMSG_CLIENT_CONFIG;
      message.replace("%1", getConfigJson(true));
      mmNetwork->sendMessage(message);
    }
}

void MMConfig::onServerMessage(String message){
  if(message == NETMSG_SERVER_REQUESTCONFIG){
    sendConfigToServer();
  }else if(message.indexOf(NETMSG_SERVER_CONFIG) > -1){
    message.replace(NETMSG_SERVER_CONFIG, "");
    parseConfigJson(message);
    saveConfig();
    delay(5000);
    #ifdef ESP32
      ESP.restart();
    #else
      rp2040.reboot();
    #endif
  }
}

