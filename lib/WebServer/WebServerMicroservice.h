#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "config.h"
#include "SensorReader.h"
#include "SessionManager.h"

class TimbangangMicroserviceClient {
private:
  AsyncWebServer* server;
  
  // Configuration URLs
  String LOAD_BALANCER_URL;
  
  // Local data cache
  bool baseMode;
  float baseWeight;
  WeightData lastWeightData;
  float finalWeight;
  String lastRFID;
  bool systemReady;
  String stabilizationStatus;
  int remainingStabilizationTime;
  bool sessionActive;
  String sessionUserUID;
  unsigned long sessionStartTime;

public:
  TimbangangMicroserviceClient();
  void init();
  void begin();
  void handleClient();
  void loop();
  
  // Configuration methods
  void setBaseMode(bool mode);
  void setBaseWeight(float weight);
  void updateWeightData(WeightData data);
  void setRFIDStatus(String uid);
  void setSystemReady(bool ready);
  void setStabilizationStatus(String status, int remainingTime = 0);
  void setFinalWeight(float weight);
  void setSessionStatus(bool active, String uid = "");
  
  // Getters
  bool getBaseMode() { return baseMode; }
  float getBaseWeight() { return baseWeight; }
  String getWebServerIP();
  WeightData getLastWeightData() { return lastWeightData; }
  bool getSessionActive() { return sessionActive; }
  String getSessionUserUID() { return sessionUserUID; }
  
  // Local web server endpoints
  String getStatusJSON();
  String getConfigJSON();
  String getMainPageHTML();
  String getConfigPageHTML();
  
  // Configuration persistence
  void loadConfiguration();
  void saveConfiguration();
  void finishBaseCalibration(float currentWeight);
  
  // RFID user management
  bool addRFIDUserToFirebase(String uid, String name, String email);
};
