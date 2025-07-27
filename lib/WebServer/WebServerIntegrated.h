#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>  // Use regular WebServer instead of AsyncWebServer
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include "config.h"
#include "SensorReader.h"
#include "SessionManager.h"

// Use regular WebServer to avoid async_tcp conflicts
class TimbangangWebServerIntegrated {
private:
  WebServer* server;  // Regular WebServer instead of AsyncWebServer
  
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
  
  // Status tracking for better UX
  String lastLoggedOutUser;
  unsigned long lastLogoutTime;
  
  // Task management
  unsigned long lastServerCheck;
  unsigned long lastFirebaseSync;
  bool serverTaskActive;

public:
  TimbangangWebServerIntegrated();
  void init();
  void begin();
  void handleClient();  // Non-blocking client handling
  void loop();          // Optimized loop with timing control
  
  // SPIFFS methods
  bool initSPIFFS();
  
  // Configuration methods
  void setBaseMode(bool mode);
  void setBaseWeight(float weight);
  void updateWeightData(WeightData data);
  void setRFIDStatus(String uid);
  void setSystemReady(bool ready);
  void setStabilizationStatus(String status, int remainingTime = 0);
  void setFinalWeight(float weight);
  void setSessionStatus(bool active, String uid = "");
  void setLogoutStatus(String uid);
  
  // Getters
  bool getBaseMode() { return baseMode; }
  float getBaseWeight() { return baseWeight; }
  String getWebServerIP();
  WeightData getLastWeightData() { return lastWeightData; }
  bool getSessionActive() { return sessionActive; }
  String getSessionUserUID() { return sessionUserUID; }
  
  // Response methods
  String getStatusJSON();
  String getConfigJSON();
  String getSystemConfigJSON();
  String getMainPageHTML();
  // Removed getConfigPageHTML() - now integrated in main page
  
  // Configuration persistence
  void loadConfiguration();
  void saveConfiguration();
  void finishBaseCalibration(float currentWeight);
  
  // RFID user management
  bool addRFIDUserToFirebase(String uid, String name, String email);
  String getAuthorizedUsersFromFirebase();
  String getUserNameFromUID(String uid);
  
  // RFID data access methods
  String getCurrentAuthorizedUser();
  bool isWeighingAccessGranted();
  bool isRFIDUsersDataCached();
  int getCachedUsersCount();
  bool collectRFIDUsersData();
  
  // Task management methods
  void enableServerTask(bool enable) { serverTaskActive = enable; }
  bool isServerTaskActive() { return serverTaskActive; }
  void optimizedFirebaseSync();
  
  // Essential handler methods only
  void handleBaseMode();
  void handleSystemTare();
  void handleFactoryReset();
  void handleTestBuzzer();
  void handleWiFiConfig();
  void handleWiFiScan();
  void handleWiFiTest();
};

extern TimbangangWebServerIntegrated webServer;
