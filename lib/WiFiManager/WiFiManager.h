#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include "config.h"

class WiFiManager {
private:
    String savedSSID;
    String savedPassword;
    bool wifiConfigured;
    
    void writeStringToEEPROM(int address, String data, int maxLength);
    String readStringFromEEPROM(int address, int maxLength);

public:
    WiFiManager();
    void init();
    
    // Configuration methods
    bool saveWiFiCredentials(String ssid, String password);
    bool loadWiFiCredentials();
    void clearWiFiCredentials();
    bool isWiFiConfigured();
    
    // Connection methods
    bool connectToWiFi();
    bool connectWithCredentials(String ssid, String password);
    
    // Getters
    String getSSID() { return savedSSID; }
    String getPassword() { return savedPassword; }
    bool hasValidCredentials();
    
    // Fallback AP mode
    void startAccessPoint();
    void stopAccessPoint();
    bool isAccessPointMode();
};

extern WiFiManager wifiManager;
