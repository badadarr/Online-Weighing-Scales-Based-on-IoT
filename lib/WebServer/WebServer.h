#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "config.h"
#include "SensorReader.h"

class TimbangangConfigServer {
private:
  AsyncWebServer* server;
  bool baseMode;
  float baseWeight;
  WeightData lastWeightData;
  float finalWeight; // Berat final setelah koreksi base
  String lastRFID;
  bool systemReady;
  String stabilizationStatus;
  int remainingStabilizationTime;

public:
  TimbangangConfigServer();
  void init();
  void begin();
  void handleClient();
  
  // Configuration methods
  void setBaseMode(bool mode);
  void setBaseWeight(float weight);
  void updateWeightData(WeightData data);
  void setRFIDStatus(String uid);
  void setSystemReady(bool ready);
  void setStabilizationStatus(String status, int remainingTime = 0);
  void setFinalWeight(float weight);
  
  // Getters
  bool getBaseMode() { return baseMode; }
  float getBaseWeight() { return baseWeight; }
  String getWebServerIP();
  WeightData getLastWeightData() { return lastWeightData; }
  
  // Configuration persistence
  void saveConfiguration();
  void loadConfiguration();
  void finishBaseCalibration(float weight);
  void startSystemCalibration(float weight);
  void stopDataSending();
  
  // Simple HTML generators
  String getMainPageHTML();
  String getConfigPageHTML();
  String getSystemPageHTML();
  String getStatusJSON();
  String getConfigJSON();
  
  bool authenticate();
  bool startBaseCalibration();
};

// Global web server instance
extern TimbangangConfigServer webServer;
