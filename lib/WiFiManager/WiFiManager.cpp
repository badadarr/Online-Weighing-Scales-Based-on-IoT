#include "WiFiManager.h"
#include "lcd_display.h"
#include "Indicator.h"

WiFiManager wifiManager;

WiFiManager::WiFiManager() {
    wifiConfigured = false;
    savedSSID = "redmi_9";
    savedPassword = "astagfirullah";
}

void WiFiManager::init() {
    EEPROM.begin(EEPROM_SIZE);
    loadWiFiCredentials();
}

void WiFiManager::writeStringToEEPROM(int address, String data, int maxLength) {
    int dataLength = data.length();
    if (dataLength > maxLength - 1) {
        dataLength = maxLength - 1; // Reserve space for null terminator
    }
    
    // Write string length first
    EEPROM.write(address, dataLength);
    
    // Write string data
    for (int i = 0; i < dataLength; i++) {
        EEPROM.write(address + 1 + i, data[i]);
    }
    
    // Write null terminator
    EEPROM.write(address + 1 + dataLength, '\0');
    
    // Fill remaining space with zeros
    for (int i = dataLength + 2; i < maxLength; i++) {
        EEPROM.write(address + i, 0);
    }
    
    EEPROM.commit();
}

String WiFiManager::readStringFromEEPROM(int address, int maxLength) {
    int dataLength = EEPROM.read(address);
    if (dataLength >= maxLength || dataLength < 0) {
        return "";
    }
    
    String result = "";
    for (int i = 0; i < dataLength; i++) {
        char c = EEPROM.read(address + 1 + i);
        if (c == '\0') break;
        result += c;
    }
    
    return result;
}

bool WiFiManager::saveWiFiCredentials(String ssid, String password) {
    if (ssid.length() == 0 || ssid.length() > 63) {
        Serial.println("[WiFiManager] Invalid SSID length");
        return false;
    }
    
    if (password.length() < 8 || password.length() > 63) {
        Serial.println("[WiFiManager] Invalid password length (must be 8-63 characters)");
        return false;
    }
    
    // Save SSID
    writeStringToEEPROM(EEPROM_ADDR_WIFI_SSID, ssid, 64);
    
    // Save password  
    writeStringToEEPROM(EEPROM_ADDR_WIFI_PASSWORD, password, 64);
    
    // Mark as configured
    EEPROM.write(EEPROM_ADDR_WIFI_CONFIGURED, 1);
    EEPROM.commit();
    
    // Update local variables
    savedSSID = ssid;
    savedPassword = password;
    wifiConfigured = true;
    
    Serial.println("[WiFiManager] WiFi credentials saved: " + ssid);
    return true;
}

bool WiFiManager::loadWiFiCredentials() {
    // Check if WiFi is configured
    wifiConfigured = (EEPROM.read(EEPROM_ADDR_WIFI_CONFIGURED) == 1);
    
    if (!wifiConfigured) {
        // Use default credentials from config.h
        savedSSID = WIFI_SSID;
        savedPassword = WIFI_PASSWORD;
        Serial.println("[WiFiManager] Using default WiFi credentials");
        return true;
    }
    
    // Load saved credentials
    savedSSID = readStringFromEEPROM(EEPROM_ADDR_WIFI_SSID, 64);
    savedPassword = readStringFromEEPROM(EEPROM_ADDR_WIFI_PASSWORD, 64);
    
    if (savedSSID.length() == 0) {
        // Fallback to default if corrupted
        savedSSID = WIFI_SSID;
        savedPassword = WIFI_PASSWORD;
        wifiConfigured = false;
        Serial.println("[WiFiManager] Corrupted credentials, using defaults");
        return false;
    }
    
    Serial.println("[WiFiManager] Loaded WiFi credentials: " + savedSSID);
    return true;
}

void WiFiManager::clearWiFiCredentials() {
    // Clear EEPROM data
    for (int i = EEPROM_ADDR_WIFI_SSID; i < EEPROM_ADDR_WIFI_SSID + 64; i++) {
        EEPROM.write(i, 0);
    }
    for (int i = EEPROM_ADDR_WIFI_PASSWORD; i < EEPROM_ADDR_WIFI_PASSWORD + 64; i++) {
        EEPROM.write(i, 0);
    }
    EEPROM.write(EEPROM_ADDR_WIFI_CONFIGURED, 0);
    EEPROM.commit();
    
    // Reset local variables
    savedSSID = WIFI_SSID;
    savedPassword = WIFI_PASSWORD;
    wifiConfigured = false;
    
    Serial.println("[WiFiManager] WiFi credentials cleared");
}

bool WiFiManager::isWiFiConfigured() {
    return wifiConfigured;
}

bool WiFiManager::hasValidCredentials() {
    return (savedSSID.length() > 0 && savedPassword.length() >= 8);
}

bool WiFiManager::connectToWiFi() {
    if (!hasValidCredentials()) {
        Serial.println("[WiFiManager] No valid credentials");
        return false;
    }
    
    return connectWithCredentials(savedSSID, savedPassword);
}

bool WiFiManager::connectWithCredentials(String ssid, String password) {
    if (ssid.length() == 0 || password.length() == 0) {
        Serial.println("[WiFiManager] Empty credentials");
        return false;
    }
    
    Serial.println("[WiFiManager] Connecting to: " + ssid);
    lcdShowStatus("Menghubungkan WiFi...");
    LEDBuzz(50);
    
    WiFi.disconnect();
    delay(100);
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    const int maxAttempts = 20; // 20 seconds timeout
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        Serial.print(".");
        lcdShowStatus("Menghubungkan... " + String(maxAttempts - attempts));
        setColor(255, 255, 0); // Yellow while connecting
        buzz(50);
        delay(1000);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFiManager] Connected to: " + ssid);
        Serial.println("[WiFiManager] IP: " + WiFi.localIP().toString());
        lcdShowStatus("Koneksi Sukses!");
        setColor(0, 255, 0); // Green on success
        delay(1000);
        setColor(0, 0, 0); // Turn off LED
        return true;
    } else {
        Serial.println("\n[WiFiManager] Failed to connect to: " + ssid);
        lcdShowStatus("WiFi Gagal!");
        setColor(255, 0, 0); // Red on failure
        buzz(200);
        delay(2000);
        setColor(0, 0, 0);
        return false;
    }
}

void WiFiManager::startAccessPoint() {
    WiFi.mode(WIFI_AP);
    String apName = "Timbangan-IoT-" + String(random(1000, 9999));
    String apPassword = "timbangan123";
    
    if (WiFi.softAP(apName.c_str(), apPassword.c_str())) {
        Serial.println("[WiFiManager] Access Point started");
        Serial.println("[WiFiManager] AP Name: " + apName);
        Serial.println("[WiFiManager] AP Password: " + apPassword);
        Serial.println("[WiFiManager] AP IP: " + WiFi.softAPIP().toString());
        
        lcdShowStatus("AP Mode");
        lcdShowStatus("IP: " + WiFi.softAPIP().toString());
        setColor(0, 0, 255); // Blue for AP mode
    } else {
        Serial.println("[WiFiManager] Failed to start Access Point");
        lcdShowStatus("AP Gagal!");
        setColor(255, 0, 0);
    }
}

void WiFiManager::stopAccessPoint() {
    WiFi.softAPdisconnect(true);
    Serial.println("[WiFiManager] Access Point stopped");
}

bool WiFiManager::isAccessPointMode() {
    return (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
}
