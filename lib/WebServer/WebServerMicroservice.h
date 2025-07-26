/**
 * @file WebServerMicroservice.h
 * @brief Header untuk web server microservice timbangan IoT
 * @details Menyediakan REST API dan web interface untuk konfigurasi dan monitoring
 *          timbangan secara remote melalui browser web
 */

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include "config.h"
#include "SensorReader.h"
#include "SessionManager.h"

/**
 * @brief Class untuk web server microservice timbangan
 * @details Mengelola web server, REST API, dan interface untuk konfigurasi
 *          serta monitoring timbangan melalui browser web
 */
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
  /**
   * @brief Constructor untuk TimbangangMicroserviceClient
   * @details Inisialisasi server dan variabel default
   */
  TimbangangMicroserviceClient();
  
  /**
   * @brief Inisialisasi web server dan endpoint API
   * @details Setup semua route REST API, static file serving, dan handler
   */
  void init();
  
  /**
   * @brief Memulai web server
   * @details Mengaktifkan web server pada port yang ditentukan
   */
  void begin();
  
  /**
   * @brief Handle client requests (compatibility method)
   * @details AsyncWebServer menangani client secara otomatis
   */
  void handleClient();
  
  /**
   * @brief Loop method untuk maintenance web server
   * @details Pemeliharaan rutin web server dan koneksi
   */
  void loop();
  
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
  String getSystemConfigJSON();
  String getMainPageHTML();
  String getConfigPageHTML();
  
  // Configuration persistence
  void loadConfiguration();
  void saveConfiguration();
  void finishBaseCalibration(float currentWeight);
  
  // RFID user management
  bool addRFIDUserToFirebase(String uid, String name, String email);
  String getAuthorizedUsersFromFirebase();
  
  // RFID data access methods
  String getCurrentAuthorizedUser();
  bool isWeighingAccessGranted();
  bool isRFIDUsersDataCached();
  int getCachedUsersCount();
  bool collectRFIDUsersData();
};
