/*
  MMStats.h - Library for Monkee Mods stats collection
  Created by sasaug
*/
#ifndef MMStats_h
#define MMStats_h

#include "Arduino.h"
#include <ArduinoJson.h>

#define MMSTATS_VERSION "1.0.0"
#define MAX_STATS_VARIABLES_SIZE 50
#define MAX_STATS_JSON_SIZE 10240
#define STATS_FILE_LOCATION "/stats.json"
#define STATS_SAVE_FREQUENCY 10000

struct Stats{
  String key;
  String type;

  //string
  String valString;

  //integer
  int valInt;
  int *arrayInt;
  int arrayIntIndex = 0;
  int arrayIntSize;
  bool arrayIntOverlapped = false;

  //long
  long valLong;

  //double
  double valDouble;

  //boolean
  bool valBool;

  //default constructor
  Stats(){
    this->key = "default";
    this->type = "string";
    this->valString = "";
  }

  //general constructor
  Stats(String key){
    this->key = key;
  }

  void setString(String value){
    this->type = "int";
    this->valString = value;
  }

  void setInt(int value){
    this->type = "int";
    this->valInt = value;
  }

  void setupIntArray(int size){
    this->type = "int_array";
    this->arrayInt = new int(size);
    this->arrayIntSize = size;
    for(int i = 0; i < size; i++){
      this->arrayInt[i] = 0;
    }
  }

  void addToIntArray(int value){
    this->arrayInt[arrayIntIndex] = value;
    arrayIntIndex++;

    if(arrayIntIndex >= arrayIntSize){
      arrayIntIndex = 0;
      arrayIntOverlapped = true;
    }
  }

  void setLong(long value){
    this->type = "long";
    this->valLong = value;
  }

  void setDouble(double value){
    this->type = "double";
    this->valDouble = value;
  }

  void setBool(bool value){
    this->type = "bool";
    this->valBool = value;
  }
};


class MMStats{
  public:
	  MMStats();
    void setup();
    void loop();
    String getStatsJson();
    void parseStatsJson(String json);
    void loadStats();
    void saveStats();
    bool haveStats(String name);
    void setStatsString(String name, String value);
    void setStatsInt(String name, int value);
    void setStatsLong(String name, long value);
    void setStatsDouble(String name, double value);
    void setStatsBool(String name, bool value);
    String getStatsString(String name);
    int getStatsInt(String name);
    long getStatsLong(String name);
    double getStatsDouble(String name);
    bool getStatsBool(String name);
    int convertStringToArray(char** list, String s, String seperator);
    void clearStats();

  private:
    //variables
    int variablesCount = 0;
    Stats variables[MAX_STATS_VARIABLES_SIZE];
    long lastSavedTime = millis();
};


#endif