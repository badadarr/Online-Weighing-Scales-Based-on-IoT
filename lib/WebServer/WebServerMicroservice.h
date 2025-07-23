#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include "config.h"
#include "SensorReader.h"
#include "SessionManager.h"

class TimbangangMicroserviceClient {
private:
  AsyncWebServer* server;
  HTTPClient http;
  
  // Microservice URLs
  String API_SERVER_URL;
  String STATIC_SERVER_URL;
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
  
  // Connection status
  bool apiConnected;
  unsigned long reconnectTimer;

public:
  TimbangangMicroserviceClient();
  void init();
  void begin();
  void handleClient();
  void loop(); // For WebSocket and connection management
  
  // Microservice communication
  bool sendWeightData(WeightData data);
  bool updateConfiguration(bool baseMode, float baseWeight);
  bool sendCalibrationRequest(float weight);
  bool sendTareRequest();
  bool sendResetRequest();
  bool sendSessionStart(String userUID);
  bool sendSessionEnd();
  
  // Connection management
  void reconnectServices();
  
  // Configuration methods (keep for backward compatibility)
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
  bool isConnectedToServices() { return apiConnected; }
  
  // Local web server endpoints (for local configuration)
  String getStatusJSON();
  String getConfigJSON();
  String getMainPageHTML();
  String getConfigPageHTML();
  
  // Configuration persistence
  void loadConfiguration();
  void saveConfiguration();
  void finishBaseCalibration(float currentWeight);
};
