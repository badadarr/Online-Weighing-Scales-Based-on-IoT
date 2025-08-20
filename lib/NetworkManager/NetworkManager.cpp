// File: NetworkManager.cpp

#include <WiFi.h>
#include "config.h"
#include "NetworkManager.h"
#include "WiFiManager.h"
#include "lcd_display.h"
#include "indicator.h"

void setupWiFiManager()
{
  wifiManager.init();
}

bool connectWiFiWithConfig()
{
  // Try to connect with saved or default credentials
  if (wifiManager.connectToWiFi())
  {
    return true;
  }

  // If connection fails, try with default credentials as fallback
  Serial.println("[NetworkManager] Trying default credentials as fallback");
  if (wifiManager.connectWithCredentials(WIFI_SSID, WIFI_PASSWORD))
  {
    return true;
  }

  // If all fails, start Access Point mode
  Serial.println("[NetworkManager] Starting Access Point mode");
  wifiManager.startAccessPoint();
  return false;
}

void connectWiFi()
{
  setupWiFiManager();

  Serial.println("[NetworkManager] Starting WiFi connection");
  lcdShowStatus("Memulai WiFi...");

  if (connectWiFiWithConfig())
  {
    Serial.println("[NetworkManager] WiFi connected successfully");
    lcdShowStatus("WiFi Terhubung!");
  }
  else
  {
    Serial.println("[NetworkManager] WiFi connection failed, AP mode active");
    lcdShowStatus("Mode AP Aktif");
  }
}
