#include "WebServerIntegrated.h"
#include "pinManager.h"
#include "Indicator.h"
#include <SPIFFS.h>
#include "config.h"
#include "SensorReader.h"
#include "SessionManager.h"
#include "RFIDReader.h"
#include "WiFiManager.h"
#include <EEPROM.h>
#include <Firebase_ESP_Client.h>

TimbangangWebServerIntegrated webServer;

TimbangangWebServerIntegrated::TimbangangWebServerIntegrated()
{
    server = new WebServer(WEB_SERVER_PORT);

    // Initialize local cache
    baseMode = DEFAULT_BASE_MODE;
    baseWeight = DEFAULT_BASE_WEIGHT;
    lastWeightData = {0.0, 0.0, 0.0, false, false, "stable", 0};
    finalWeight = 0.0;
    lastRFID = "";
    systemReady = false;
    stabilizationStatus = "standby";
    remainingStabilizationTime = 0;
    sessionActive = false;
    sessionUserUID = "";
    sessionStartTime = 0;
    
    // Initialize status tracking
    lastLoggedOutUser = "";
    lastLogoutTime = 0;

    // Task timing
    lastServerCheck = 0;
    lastFirebaseSync = 0;
    serverTaskActive = false;
}

void TimbangangWebServerIntegrated::init()
{
    // Initialize SPIFFS first
    if (!initSPIFFS())
    {
        Serial.println("[SPIFFS] Failed to initialize SPIFFS");
    }

    loadConfiguration();

    // Setup web server routes with reduced complexity
    server->on("/", [this]()
               { 
                   if (SPIFFS.exists("/index.html")) {
                       File file = SPIFFS.open("/index.html", "r");
                       server->streamFile(file, "text/html");
                       file.close();
                   } else {
                       server->send(200, "text/html", getMainPageHTML());
                   } });

    // Remove config.html route - now integrated in main page

    // Serve static files from SPIFFS
    server->on("/style.css", [this]()
               {
    if (SPIFFS.exists("/style.css")) {
      File file = SPIFFS.open("/style.css", "r");
      server->streamFile(file, "text/css");
      file.close();
    } else {
      server->send(404, "text/plain", "CSS not found");
    } });

    server->on("/script.js", [this]()
               {
    if (SPIFFS.exists("/script.js")) {
      File file = SPIFFS.open("/script.js", "r");
      server->streamFile(file, "application/javascript");
      file.close();
    } else {
      server->send(404, "text/plain", "JS not found");
    } });

    // Essential API endpoints only
    server->on("/weight", HTTP_GET, [this]()
               { server->send(200, "text/plain", String(finalWeight, 3)); });

    server->on("/status", HTTP_GET, [this]()
               { server->send(200, "application/json", getStatusJSON()); });

    server->on("/api/status", HTTP_GET, [this]()
               { server->send(200, "application/json", getStatusJSON()); });

    server->on("/api/config", HTTP_GET, [this]()
               { server->send(200, "application/json", getConfigJSON()); });

    // Essential system endpoints only
    server->on("/api/base-mode", HTTP_POST, [this]()
               { handleBaseMode(); });

    server->on("/api/system-tare", HTTP_POST, [this]()
               { handleSystemTare(); });

    server->on("/api/factory-reset", HTTP_POST, [this]()
               { handleFactoryReset(); });

    server->on("/api/test-buzzer", HTTP_POST, [this]()
               { handleTestBuzzer(); });





    // Essential tare endpoint
    server->on("/api/tare", HTTP_POST, [this]()
               {
    extern bool webTareRequested;
    webTareRequested = true;
    server->send(200, "application/json", 
      "{\"status\":\"success\",\"message\":\"Tare requested\"}"); });

    // Reset endpoint
    server->on("/api/reset", HTTP_POST, [this]()
               {
    baseMode = DEFAULT_BASE_MODE;
    baseWeight = DEFAULT_BASE_WEIGHT;
    saveConfiguration();
    server->send(200, "application/json", "{\"status\":\"success\"}"); });

    // System configuration endpoints
    server->on("/api/system-config", HTTP_GET, [this]()
               { server->send(200, "application/json", getSystemConfigJSON()); });

    server->on("/api/system-config", HTTP_POST, [this]()
               {
    if (!server->hasArg("plain")) {
      server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data received\"}");
      return;
    }
    
    String body = server->arg("plain");
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
      server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
      return;
    }
    
    // Here you would save system configuration
    // For now, just return success
    server->send(200, "application/json", "{\"status\":\"success\",\"message\":\"System configuration saved\"}"); });

    // RFID users endpoints
    server->on("/api/rfid/users", HTTP_GET, [this]()
               {
    // Get users from Firebase or local cache
    String usersData = getAuthorizedUsersFromFirebase();
    if (usersData.length() > 0) {
      server->send(200, "application/json", "{\"success\":true,\"users\":" + usersData + "}");
    } else {
      server->send(200, "application/json", "{\"success\":false,\"users\":[],\"message\":\"No users found\"}");
    } });

    server->on("/api/add-rfid-user", HTTP_POST, [this]()
               {
    if (!server->hasArg("plain")) {
      server->send(400, "application/json", "{\"success\":false,\"message\":\"No data received\"}");
      return;
    }
    
    String body = server->arg("plain");
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
      server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
      return;
    }
    
    String uid = doc["uid"].as<String>();
    String name = doc["name"].as<String>();
    String email = doc["email"].as<String>();
    
    if (uid.length() == 0 || name.length() == 0) {
      server->send(400, "application/json", "{\"success\":false,\"message\":\"UID and name are required\"}");
      return;
    }
    
    bool success = addRFIDUserToFirebase(uid, name, email);
    if (success) {
      server->send(200, "application/json", "{\"success\":true,\"message\":\"User added successfully\"}");
    } else {
      server->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to add user\"}");
    } });

    // RFID sync endpoint
    server->on("/api/rfid-sync", HTTP_POST, [this]()
               {
    bool success = collectRFIDUsersData();
    
    if (success) {
      int count = getCachedUsersCount();
      server->send(200, "application/json", 
        "{\"status\":\"success\",\"message\":\"RFID data synced\",\"cachedUsers\":" + String(count) + "}");
    } else {
      server->send(500, "application/json", 
        "{\"status\":\"error\",\"message\":\"Failed to sync RFID data\"}");
    } });

    // WiFi configuration endpoints
    server->on("/api/wifi-config", HTTP_GET, [this]()
               {
    DynamicJsonDocument doc(256);
    doc["currentSSID"] = WiFi.SSID();
    doc["isConnected"] = (WiFi.status() == WL_CONNECTED);
    doc["ipAddress"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["isAPMode"] = wifiManager.isAccessPointMode();
    doc["savedSSID"] = wifiManager.getSSID();
    doc["isConfigured"] = wifiManager.isWiFiConfigured();
    
    String output;
    serializeJson(doc, output);
    server->send(200, "application/json", output); });

    server->on("/api/wifi-config", HTTP_POST, [this]()
               { handleWiFiConfig(); });

    server->on("/api/wifi-scan", HTTP_GET, [this]()
               { handleWiFiScan(); });

    server->on("/api/wifi-test", HTTP_POST, [this]()
               { handleWiFiTest(); });

    // 404 handler
    server->onNotFound([this]()
                       { server->send(404, "text/html", "Not Found"); });

    Serial.println("[WEB] Integrated web server initialized");
}

void TimbangangWebServerIntegrated::begin()
{
    server->begin();
    serverTaskActive = true;
    Serial.println("[WEB] Local web server started on port " + String(WEB_SERVER_PORT));
    Serial.println("[WEB] Local access: http://" + WiFi.localIP().toString());
}

void TimbangangWebServerIntegrated::handleClient()
{
    // Non-blocking client handling with timing control
    unsigned long currentTime = millis();

    if (serverTaskActive && currentTime - lastServerCheck > 50)
    { // Check every 50ms
        server->handleClient();
        lastServerCheck = currentTime;

        // Yield to other tasks
        yield();
        delay(1);
    }
}

void TimbangangWebServerIntegrated::loop()
{
    // Optimized loop with task management
    handleClient();

    // Optional Firebase sync with longer intervals
    unsigned long currentTime = millis();
    if (currentTime - lastFirebaseSync > 10000)
    { // Every 10 seconds
        optimizedFirebaseSync();
        lastFirebaseSync = currentTime;
    }
}

void TimbangangWebServerIntegrated::optimizedFirebaseSync()
{
    // Optional Firebase operations - can be disabled if causing issues
    // This method intentionally left minimal to avoid SSL conflicts
    Serial.println("[WEB] Firebase sync skipped - preventing SSL conflicts");
}

// Implementation of setter methods
void TimbangangWebServerIntegrated::setBaseMode(bool mode)
{
    baseMode = mode;
}

void TimbangangWebServerIntegrated::setBaseWeight(float weight)
{
    baseWeight = weight;
}

void TimbangangWebServerIntegrated::updateWeightData(WeightData data)
{
    lastWeightData = data;

    // Calculate final weight with base correction
    if (baseMode && baseWeight > 0)
    {
        finalWeight = max(0.0f, data.filtered - baseWeight);
    }
    else
    {
        finalWeight = data.filtered;
    }
}

void TimbangangWebServerIntegrated::setRFIDStatus(String uid)
{
    lastRFID = uid;
}

void TimbangangWebServerIntegrated::setSystemReady(bool ready)
{
    systemReady = ready;
}

void TimbangangWebServerIntegrated::setStabilizationStatus(String status, int remainingTime)
{
    stabilizationStatus = status;
    remainingStabilizationTime = remainingTime;
}

void TimbangangWebServerIntegrated::setFinalWeight(float weight)
{
    finalWeight = weight;
}

void TimbangangWebServerIntegrated::setSessionStatus(bool active, String uid)
{
    sessionActive = active;
    sessionUserUID = uid;
    if (active)
    {
        sessionStartTime = millis();
    }
    else
    {
        sessionStartTime = 0;
    }
}

void TimbangangWebServerIntegrated::setLogoutStatus(String uid)
{
    lastLoggedOutUser = uid;
    lastLogoutTime = millis();
}

String TimbangangWebServerIntegrated::getWebServerIP()
{
    return WiFi.localIP().toString();
}

// JSON response methods (simplified versions)
String TimbangangWebServerIntegrated::getStatusJSON()
{
    String authorizedUser = getCurrentAuthorizedUser();
    bool accessGranted = isWeighingAccessGranted();
    bool rfidDataCached = isRFIDUsersDataCached();
    int cachedUsers = getCachedUsersCount();

    DynamicJsonDocument doc(512);
    doc["weight"] = finalWeight;
    doc["rawWeight"] = lastWeightData.raw;
    doc["baseMode"] = baseMode;
    doc["baseWeight"] = baseWeight;
    doc["lastRFID"] = lastRFID;
    doc["systemReady"] = systemReady;
    doc["isStable"] = lastWeightData.isStable;
    doc["stabilizationStatus"] = stabilizationStatus;
    doc["remainingTime"] = remainingStabilizationTime;
    doc["accessGranted"] = accessGranted;
    doc["authorizedUser"] = authorizedUser;
    doc["rfidDataCached"] = rfidDataCached;
    doc["cachedUsers"] = cachedUsers;

    // Add status fields for the web interface
    if (sessionActive && !sessionUserUID.isEmpty())
    {
        String userName = getUserNameFromUID(sessionUserUID);
        if (userName.isEmpty()) {
            userName = "User " + sessionUserUID.substring(0, 4);
        }
        doc["access_status"] = "Akses diberikan - Selamat datang " + userName;
    }
    else if (!lastLoggedOutUser.isEmpty() && (millis() - lastLogoutTime) < 10000) // Show logout message for 10 seconds
    {
        String userName = getUserNameFromUID(lastLoggedOutUser);
        if (userName.isEmpty()) {
            userName = "User " + lastLoggedOutUser.substring(0, 4);
        }
        doc["access_status"] = "User " + userName + " telah logout";
    }
    else if (!lastRFID.isEmpty())
    {
        String userName = getUserNameFromUID(lastRFID);
        if (userName.isEmpty()) {
            doc["access_status"] = "Akses ditolak - RFID tidak dikenal";
        } else {
            doc["access_status"] = "Akses ditolak untuk " + userName;
        }
    }
    else
    {
        doc["access_status"] = "Tap kartu RFID untuk mengakses timbangan";
    }

    if (rfidDataCached)
    {
        doc["rfid_status"] = String(cachedUsers) + " users loaded";
    }
    else
    {
        doc["rfid_status"] = "Loading user database...";
    }

    String output;
    serializeJson(doc, output);
    return output;
}

String TimbangangWebServerIntegrated::getConfigJSON()
{
    DynamicJsonDocument doc(1024);

    // Configuration data
    doc["baseMode"] = baseMode ? "on" : "off";
    doc["baseWeight"] = baseWeight;
    doc["calibrationFactor"] = 1.0;
    doc["stabilizationTime"] = 3;
    doc["weightThreshold"] = 1.0;

    // System information
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000; // in seconds
    doc["wifiRSSI"] = WiFi.RSSI();
    doc["ipAddress"] = WiFi.localIP().toString();
    doc["systemReady"] = systemReady;

    // Session information
    doc["sessionActive"] = sessionActive;
    doc["sessionUserUID"] = sessionUserUID;
    doc["accessGranted"] = isWeighingAccessGranted();
    doc["authorizedUser"] = getCurrentAuthorizedUser();
    doc["cachedUsers"] = getCachedUsersCount();

    String output;
    serializeJson(doc, output);
    return output;
}

String TimbangangWebServerIntegrated::getSystemConfigJSON()
{
    DynamicJsonDocument doc(512);
    doc["wifiSSID"] = wifiManager.getSSID();
    doc["currentSSID"] = WiFi.SSID();
    doc["isConnected"] = (WiFi.status() == WL_CONNECTED);
    doc["ipAddress"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    doc["isAPMode"] = wifiManager.isAccessPointMode();
    doc["isConfigured"] = wifiManager.isWiFiConfigured();
    doc["sessionTimeout"] = 5;
    doc["serverURL"] = DATABASE_URL;

    String output;
    serializeJson(doc, output);
    return output;
}

bool TimbangangWebServerIntegrated::initSPIFFS()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("[SPIFFS] An Error has occurred while mounting SPIFFS");
        return false;
    }
    Serial.println("[SPIFFS] SPIFFS mounted successfully");

    // List files in SPIFFS for debugging
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    Serial.println("[SPIFFS] Files in filesystem:");
    while (file)
    {
        Serial.print("[SPIFFS] FILE: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }

    return true;
}

// HTML pages (minimal versions to save flash memory)
String TimbangangWebServerIntegrated::getMainPageHTML()
{
    return R"HTML(<!DOCTYPE html><html><head><title>IoT Scale</title><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{font-family:Arial;margin:20px;text-align:center}.container{max-width:400px;margin:0 auto;padding:20px;border:1px solid #ddd;border-radius:10px}.weight{font-size:24px;font-weight:bold;color:#333;margin:10px 0}.base-info{font-size:14px;color:#666;margin:5px 0}.access-status{padding:10px;margin:10px 0;border-radius:5px}.access-granted{background:#d4edda;color:#155724;border:1px solid #c3e6cb}.access-denied{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}.rfid-status{padding:8px;margin:5px 0;border-radius:5px;font-size:12px}.btn{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:5px;margin:5px;cursor:pointer;text-decoration:none;display:inline-block}.btn-warning{background:#ffc107;color:#212529}.btn:hover{opacity:0.8}</style></head><body><div class="container"><h1>IoT Scale</h1><div class="weight" id="weight">Loading...</div><div class="base-info" id="base-info">Mode: Loading...</div><div>Status: <span id="status">Ready</span></div><div id="access-status" class="access-status"></div><div id="rfid-status" class="rfid-status"></div><div><button class="btn btn-warning" onclick="quickTare()">Tare</button><button class="btn" onclick="setBaseMode()">Mode Base</button><button class="btn" onclick="setNormalMode()">Mode Normal</button></div></div><script>document.addEventListener("DOMContentLoaded",function(){let isConnected=false;updateWeight();setInterval(updateWeight,1000);setInterval(updateStatus,2000);function updateWeight(){fetch("/weight").then(response=>response.text()).then(data=>{const weightElement=document.getElementById("weight");if(weightElement){weightElement.textContent=data+" kg"}isConnected=true}).catch(error=>{console.error("Error fetching weight:",error);const weightElement=document.getElementById("weight");if(weightElement){weightElement.textContent="Connection Error"}isConnected=false})}function updateStatus(){fetch("/status").then(response=>response.json()).then(data=>{const statusElement=document.getElementById("status");const accessElement=document.getElementById("access-status");const rfidElement=document.getElementById("rfid-status");const baseInfoElement=document.getElementById("base-info");if(statusElement){statusElement.textContent=isConnected?"Connected":"Disconnected";statusElement.style.color=isConnected?"#28a745":"#dc3545"}if(baseInfoElement){if(data.baseMode&&data.baseWeight>0){baseInfoElement.textContent="Mode: Base ("+data.baseWeight.toFixed(3)+" kg)";baseInfoElement.style.color="#007bff"}else{baseInfoElement.textContent="Mode: Normal";baseInfoElement.style.color="#6c757d"}}if(accessElement&&data.access_status){accessElement.textContent=data.access_status;accessElement.className="access-status";if(data.access_status.includes("granted")||data.access_status.includes("welcome")){accessElement.classList.add("granted")}else if(data.access_status.includes("denied")||data.access_status.includes("unauthorized")){accessElement.classList.add("denied")}}if(rfidElement&&data.rfid_status){rfidElement.textContent="RFID: "+data.rfid_status}}).catch(error=>{console.error("Error fetching status:",error);const statusElement=document.getElementById("status");if(statusElement){statusElement.textContent="Error";statusElement.style.color="#dc3545"}})}});function quickTare(){if(!confirm("This will set the current reading as zero. Make sure the scale is empty. Continue?")){return}fetch("/api/tare",{method:"POST"}).then(response=>response.json()).then(data=>{if(data.status==="success"){alert("Scale tared successfully!")}else{alert("Error performing tare: "+(data.message||"Unknown error"))}}).catch(error=>{console.error("Error performing tare:",error);alert("Network error. Please try again.")})};function setBaseMode(){if(!confirm("Aktifkan Mode Base? Letakkan wadah/base di timbangan dan pastikan stabil.")){return}fetch("/api/base-mode",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({mode:"on"})}).then(response=>response.json()).then(data=>{if(data.status==="success"){alert("Mode Base diaktifkan!")}else{alert("Error: "+(data.message||"Unknown error"))}}).catch(error=>{console.error("Error setting base mode:",error);alert("Kesalahan jaringan. Silakan coba lagi.")})};function setNormalMode(){if(!confirm("Nonaktifkan Mode Base dan kembali ke Mode Normal?")){return}fetch("/api/base-mode",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({mode:"off"})}).then(response=>response.json()).then(data=>{if(data.status==="success"){alert("Mode Normal diaktifkan!")}else{alert("Error: "+(data.message||"Unknown error"))}}).catch(error=>{console.error("Error setting normal mode:",error);alert("Kesalahan jaringan. Silakan coba lagi.")})}</script></body></html>)HTML";
}

// Configuration persistence methods
void TimbangangWebServerIntegrated::loadConfiguration()
{
    EEPROM.begin(512);

    // Read base mode
    uint8_t mode = EEPROM.read(EEPROM_ADDR_BASE_MODE);
    if (mode == 0 || mode == 1)
    {
        baseMode = (mode == 1);
    }

    // Read base weight
    EEPROM.get(EEPROM_ADDR_BASE_WEIGHT, baseWeight);
    if (isnan(baseWeight) || baseWeight < 0 || baseWeight > 50)
    {
        baseWeight = DEFAULT_BASE_WEIGHT;
    }

    Serial.println("[CONFIG] Loaded - Base Mode: " + String(baseMode ? "ON" : "OFF") +
                   ", Base Weight: " + String(baseWeight, 3) + " kg");
}

void TimbangangWebServerIntegrated::saveConfiguration()
{
    EEPROM.write(EEPROM_ADDR_BASE_MODE, baseMode ? 1 : 0);
    EEPROM.put(EEPROM_ADDR_BASE_WEIGHT, baseWeight);
    EEPROM.commit();

    Serial.println("[CONFIG] Saved - Base Mode: " + String(baseMode ? "ON" : "OFF") +
                   ", Base Weight: " + String(baseWeight, 3) + " kg");
}

void TimbangangWebServerIntegrated::finishBaseCalibration(float currentWeight)
{
    if (currentWeight > 0.01)
    { // Minimum weight threshold
        baseWeight = currentWeight;
        baseMode = true;
        saveConfiguration();

        Serial.println("[CALIBRATION] Base calibrated: " + String(baseWeight, 3) + " kg");
    }
}

bool TimbangangWebServerIntegrated::addRFIDUserToFirebase(String uid, String name, String email)
{
    // Simplified implementation to avoid Firebase conflicts
    Serial.println("[RFID] Add user request: " + uid + " - " + name);
    return true; // Placeholder - implement based on your Firebase setup
}

String TimbangangWebServerIntegrated::getAuthorizedUsersFromFirebase()
{
    // Return properly formatted JSON array of users
    // For now, return sample data - implement Firebase integration later
    return "[{\"uid\":\"039CA70D\",\"name\":\"BADAR_MAULANA_2043\",\"email\":\"badar@example.com\"},{\"uid\":\"123ABC45\",\"name\":\"OPERATOR_2263\",\"email\":\"operator@example.com\"}]";
}

String TimbangangWebServerIntegrated::getUserNameFromUID(String uid)
{
    extern FirebaseData fbdo;
    
    if (!Firebase.ready() || uid.isEmpty()) {
        return "";
    }
    
    // Try to get user name from authorized_users path
    String path = "/authorized_users/" + uid + "/name";
    if (Firebase.RTDB.getString(&fbdo, path.c_str())) {
        String name = fbdo.stringData();
        if (name.length() > 0) {
            return name;
        }
    }
    
    // Try alternative paths
    String altPaths[] = {
        "/rfid_users/" + uid + "/name",
        "/users/" + uid + "/name"
    };
    
    for (int i = 0; i < 2; i++) {
        if (Firebase.RTDB.getString(&fbdo, altPaths[i].c_str())) {
            String name = fbdo.stringData();
            if (name.length() > 0) {
                return name;
            }
        }
    }
    
    // If no name found, return empty string
    return "";
}

// RFID data access methods (implement based on your existing RFID system)
String TimbangangWebServerIntegrated::getCurrentAuthorizedUser()
{
    // Return the current session user UID
    return sessionActive ? sessionUserUID : "";
}

bool TimbangangWebServerIntegrated::isWeighingAccessGranted()
{
    // Check if we have an active session
    return sessionActive && !sessionUserUID.isEmpty();
}

bool TimbangangWebServerIntegrated::isRFIDUsersDataCached()
{
    // Simplified implementation - return true as default
    // Implement based on your existing RFID reader when needed
    return true; // Placeholder
}

int TimbangangWebServerIntegrated::getCachedUsersCount()
{
    // Return the actual count of users from the sample data
    return 2; // Based on the sample data above
}

bool TimbangangWebServerIntegrated::collectRFIDUsersData()
{
    // Simplified implementation - return success as default
    // Implement based on your existing RFID reader when needed
    Serial.println("[RFID] Data collection placeholder - implement when needed");
    return true; // Placeholder
}



void TimbangangWebServerIntegrated::handleBaseMode()
{
    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data received\"}");
        return;
    }

    String body = server->arg("plain");
    DynamicJsonDocument doc(256);
    deserializeJson(doc, body);

    String mode = doc["mode"].as<String>();
    
    if (mode == "on") {
        // Set base mode with current weight
        float currentWeight = lastWeightData.filtered;
        if (currentWeight > 0.01) {
            setBaseWeight(currentWeight);
            setBaseMode(true);
            saveConfiguration();
            Serial.println("[WEB] Base mode ON: " + String(currentWeight, 3) + " kg");
            server->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Base mode activated\"}");
        } else {
            server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No weight detected\"}");
        }
    } else if (mode == "off") {
        setBaseMode(false);
        setBaseWeight(0.0);
        saveConfiguration();
        Serial.println("[WEB] Base mode OFF");
        server->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Normal mode activated\"}");
    } else {
        server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid mode\"}");
    }
}

void TimbangangWebServerIntegrated::handleSystemTare()
{
    extern bool webTareRequested;
    webTareRequested = true;

    Serial.println("[WEB] System tare requested");

    server->send(200, "application/json",
                 "{\"status\":\"success\",\"message\":\"System tare initiated\"}");
}

void TimbangangWebServerIntegrated::handleFactoryReset()
{
    // Reset configuration to defaults
    baseMode = DEFAULT_BASE_MODE;
    baseWeight = DEFAULT_BASE_WEIGHT;

    // Clear EEPROM
    for (int i = 0; i < 512; i++)
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();

    saveConfiguration();

    Serial.println("[WEB] Factory reset completed");

    server->send(200, "application/json",
                 "{\"status\":\"success\",\"message\":\"Factory reset completed. Device will restart.\"}");

    // Restart device after a delay
    delay(1000);
    ESP.restart();
}

void TimbangangWebServerIntegrated::handleTestBuzzer()
{
    // Test buzzer and LED
    extern void buzz(int duration);
    extern void LEDBuzz(int duration);

    buzz(200);
    LEDBuzz(200);

    Serial.println("[WEB] Buzzer test completed");

    server->send(200, "application/json",
                 "{\"status\":\"success\",\"message\":\"Buzzer test completed\"}");
}

void TimbangangWebServerIntegrated::handleWiFiConfig()
{
    if (!server->hasArg("plain"))
    {
        server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data received\"}");
        return;
    }

    String body = server->arg("plain");
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    String ssid = doc["ssid"].as<String>();
    String password = doc["password"].as<String>();

    if (ssid.length() == 0)
    {
        server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID cannot be empty\"}");
        return;
    }

    if (password.length() < 8)
    {
        server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Password must be at least 8 characters\"}");
        return;
    }

    // Save WiFi credentials
    if (wifiManager.saveWiFiCredentials(ssid, password))
    {
        Serial.println("[WEB] WiFi credentials saved: " + ssid);
        server->send(200, "application/json", "{\"status\":\"success\",\"message\":\"WiFi credentials saved. Device will restart to apply changes.\"}");

        // Restart device after a delay to apply new WiFi settings
        delay(2000);
        ESP.restart();
    }
    else
    {
        server->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to save WiFi credentials\"}");
    }
}

void TimbangangWebServerIntegrated::handleWiFiScan()
{
    Serial.println("[WEB] Starting WiFi scan");

    int networkCount = WiFi.scanNetworks();

    DynamicJsonDocument doc(2048);
    JsonArray networks = doc.createNestedArray("networks");

    if (networkCount > 0)
    {
        for (int i = 0; i < networkCount && i < 20; i++)
        { // Limit to 20 networks
            JsonObject network = networks.createNestedObject();
            network["ssid"] = WiFi.SSID(i);
            network["rssi"] = WiFi.RSSI(i);
            network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
            network["channel"] = WiFi.channel(i);
        }
        doc["status"] = "success";
        doc["count"] = networkCount;
    }
    else
    {
        doc["status"] = "error";
        doc["message"] = "No networks found";
        doc["networks"] = networks; // Empty array
    }

    String output;
    serializeJson(doc, output);
    server->send(200, "application/json", output);

    WiFi.scanDelete(); // Clean up scan results
}

void TimbangangWebServerIntegrated::handleWiFiTest()
{
    if (!server->hasArg("plain"))
    {
        server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data received\"}");
        return;
    }

    String body = server->arg("plain");
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
        return;
    }

    String ssid = doc["ssid"].as<String>();
    String password = doc["password"].as<String>();

    if (ssid.length() == 0)
    {
        server->send(400, "application/json", "{\"status\":\"error\",\"message\":\"SSID cannot be empty\"}");
        return;
    }

    Serial.println("[WEB] Testing WiFi connection: " + ssid);

    // Test connection without saving credentials
    WiFi.disconnect();
    delay(100);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    const int maxAttempts = 15; // 15 seconds timeout

    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts)
    {
        delay(1000);
        attempts++;
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        String testIP = WiFi.localIP().toString();
        int testRSSI = WiFi.RSSI();

        server->send(200, "application/json",
                     "{\"status\":\"success\",\"message\":\"Connection test successful\","
                     "\"ip\":\"" +
                         testIP + "\",\"rssi\":" + String(testRSSI) + "}");

        Serial.println("[WEB] WiFi test successful: " + testIP);
    }
    else
    {
        server->send(200, "application/json",
                     "{\"status\":\"error\",\"message\":\"Failed to connect to " + ssid + "\"}");

        Serial.println("[WEB] WiFi test failed for: " + ssid);
    }

    // Reconnect to original network if we were connected
    if (wifiManager.hasValidCredentials())
    {
        delay(1000);
        wifiManager.connectToWiFi();
    }
}
