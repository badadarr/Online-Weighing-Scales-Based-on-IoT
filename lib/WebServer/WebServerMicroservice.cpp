#include "WebServerMicroservice.h"
#include "pinManager.h"
#include "Indicator.h"
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "SensorReader.h"
#include "SessionManager.h"
#include "RFIDReader.h"
#include <EEPROM.h>

TimbangangMicroserviceClient webMicroservice;

TimbangangMicroserviceClient::TimbangangMicroserviceClient() {
  server = new AsyncWebServer(WEB_SERVER_PORT);
  
  // Default microservice URLs (can be configured)
  LOAD_BALANCER_URL = "http://192.168.1.100"; // Change to your server IP
  API_SERVER_URL = LOAD_BALANCER_URL + "/api";
  STATIC_SERVER_URL = LOAD_BALANCER_URL;
  
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
  
  // Connection status
  apiConnected = false;
  reconnectTimer = 0;
}

void TimbangangMicroserviceClient::init() {
  loadConfiguration();
  
  // Setup local web server for device configuration
  server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getMainPageHTML());
  });
  
  server->on("/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getConfigPageHTML());
  });
  
  server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getStatusJSON());
  });
  
  server->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getConfigJSON());
  });
  
  // Local configuration endpoints (for device-level config)
  server->on("/api/config", HTTP_POST, [this](AsyncWebServerRequest *request) {},
    NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = String((char*)data).substring(0, len);
      DynamicJsonDocument doc(512);
      deserializeJson(doc, body);
      
      bool configChanged = false;
      
      if (doc.containsKey("baseMode")) {
        setBaseMode(doc["baseMode"]);
        configChanged = true;
      }
      if (doc.containsKey("baseWeight")) {
        setBaseWeight(doc["baseWeight"]);
        configChanged = true;
      }
      if (doc.containsKey("serverURL")) {
        LOAD_BALANCER_URL = doc["serverURL"].as<String>();
        API_SERVER_URL = LOAD_BALANCER_URL + "/api";
        STATIC_SERVER_URL = LOAD_BALANCER_URL;
        configChanged = true;
      }
      
      if (configChanged) {
        saveConfiguration();
        
        // Also send to microservice if connected
        updateConfiguration(baseMode, baseWeight);
      }
      
      request->send(200, "application/json", 
        "{\"status\":\"success\",\"message\":\"Configuration saved successfully\"}");
    });
  
  // Calibration endpoint
  server->on("/api/calibrate", HTTP_POST, [this](AsyncWebServerRequest *request) {
    float currentWeight = lastWeightData.raw > 0 ? lastWeightData.raw : lastWeightData.filtered;
    
    if (currentWeight <= 0.01) {
      request->send(400, "application/json", 
        "{\"status\":\"error\",\"message\":\"No weight detected. Place base on scale first.\"}");
      return;
    }
    
    if (!lastWeightData.isStable) {
      request->send(400, "application/json", 
        "{\"status\":\"error\",\"message\":\"Weight not stable. Wait for stable reading.\"}");
      return;
    }
    
    // Send to microservice first
    bool success = sendCalibrationRequest(currentWeight);
    
    if (success || !apiConnected) {
      // If microservice successful or not connected, do local calibration
      finishBaseCalibration(currentWeight);
      setBaseMode(true);
      
      Serial.println("[WEB] Calibration: " + String(currentWeight, 3) + " kg -> " + String(baseWeight, 3) + " kg");
      
      request->send(200, "application/json", 
        "{\"status\":\"success\",\"baseWeight\":" + String(baseWeight, WEIGHT_PRECISION) + 
        ",\"message\":\"Base calibrated successfully\"}");
    } else {
      request->send(500, "application/json", 
        "{\"status\":\"error\",\"message\":\"Failed to communicate with server\"}");
    }
  });
  
  // Tare endpoint
  server->on("/api/tare", HTTP_POST, [this](AsyncWebServerRequest *request) {
    bool success = sendTareRequest();
    
    if (success || !apiConnected) {
      extern bool webTareRequested;
      webTareRequested = true;
      request->send(200, "application/json", 
        "{\"status\":\"success\",\"message\":\"Tare requested\"}");
    } else {
      request->send(500, "application/json", 
        "{\"status\":\"error\",\"message\":\"Failed to communicate with server\"}");
    }
  });
  
  // Reset endpoint
  server->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
    bool success = sendResetRequest();
    
    if (success || !apiConnected) {
      baseMode = DEFAULT_BASE_MODE;
      baseWeight = DEFAULT_BASE_WEIGHT;
      saveConfiguration();
      request->send(200, "application/json", "{\"status\":\"success\"}");
    } else {
      request->send(500, "application/json", 
        "{\"status\":\"error\",\"message\":\"Failed to communicate with server\"}");
    }
  });
  
  // RFID data collection endpoint
  server->on("/api/rfid-sync", HTTP_POST, [this](AsyncWebServerRequest *request) {
    bool success = collectRFIDUsersData();
    
    if (success) {
      int count = getCachedUsersCount();
      request->send(200, "application/json", 
        "{\"status\":\"success\",\"message\":\"RFID data synced\",\"cachedUsers\":" + String(count) + "}");
    } else {
      request->send(500, "application/json", 
        "{\"status\":\"error\",\"message\":\"Failed to sync RFID data\"}");
    }
  });
  
  server->onNotFound([this](AsyncWebServerRequest *request) {
    request->send(404, "text/html", "Not Found");
  });
  
  Serial.println("[WEB] Microservice client initialized");
  Serial.println("[WEB] Server URL: " + LOAD_BALANCER_URL);
}

void TimbangangMicroserviceClient::begin() {
  server->begin();
  Serial.println("[WEB] Local web server started on port " + String(WEB_SERVER_PORT));
  Serial.println("[WEB] Local access: http://" + WiFi.localIP().toString());
  
  // Try initial API connection
  reconnectServices();
}

void TimbangangMicroserviceClient::loop() {
  unsigned long now = millis();
  
  // Reconnection logic (simplified - no WebSocket)
  if (!apiConnected && (now - reconnectTimer > 30000)) { // Try reconnect every 30s
    reconnectServices();
    reconnectTimer = now;
  }
}

void TimbangangMicroserviceClient::handleClient() {
  // AsyncWebServer handles clients automatically
  // This method kept for compatibility
}

bool TimbangangMicroserviceClient::sendWeightData(WeightData data) {
  if (!apiConnected) return false;
  
  http.begin(API_SERVER_URL + "/weight-data");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(512);
  doc["raw"] = data.raw;
  doc["filtered"] = data.filtered;
  doc["final"] = finalWeight; // Send final weight after base correction
  doc["isStable"] = data.isStable;
  doc["isCalibrated"] = baseMode;
  doc["status"] = stabilizationStatus;
  doc["deviceId"] = WiFi.macAddress();
  doc["timestamp"] = millis();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  bool success = (httpCode == 200);
  
  if (success) {
    Serial.println("[API] Weight data sent successfully");
  } else {
    Serial.println("[API] Failed to send weight data: " + String(httpCode));
    apiConnected = false;
  }
  
  http.end();
  return success;
}

bool TimbangangMicroserviceClient::updateConfiguration(bool bMode, float bWeight) {
  if (!apiConnected) return false;
  
  http.begin(API_SERVER_URL + "/config");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(256);
  doc["baseMode"] = bMode;
  doc["baseWeight"] = bWeight;
  doc["deviceId"] = WiFi.macAddress();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  bool success = (httpCode == 200);
  
  if (!success) {
    apiConnected = false;
  }
  
  http.end();
  return success;
}

bool TimbangangMicroserviceClient::sendCalibrationRequest(float weight) {
  if (!apiConnected) return false;
  
  http.begin(API_SERVER_URL + "/calibrate");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(256);
  doc["weight"] = weight;
  doc["deviceId"] = WiFi.macAddress();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  bool success = (httpCode == 200);
  
  if (!success) {
    apiConnected = false;
  }
  
  http.end();
  return success;
}

bool TimbangangMicroserviceClient::sendTareRequest() {
  if (!apiConnected) return false;
  
  http.begin(API_SERVER_URL + "/tare");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(128);
  doc["deviceId"] = WiFi.macAddress();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  bool success = (httpCode == 200);
  
  if (!success) {
    apiConnected = false;
  }
  
  http.end();
  return success;
}

bool TimbangangMicroserviceClient::sendResetRequest() {
  if (!apiConnected) return false;
  
  http.begin(API_SERVER_URL + "/reset");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(128);
  doc["deviceId"] = WiFi.macAddress();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  bool success = (httpCode == 200);
  
  if (!success) {
    apiConnected = false;
  }
  
  http.end();
  return success;
}

bool TimbangangMicroserviceClient::sendSessionStart(String userUID) {
  if (!apiConnected) return false;
  
  http.begin(API_SERVER_URL + "/session/start");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(256);
  doc["userUID"] = userUID;
  doc["deviceId"] = WiFi.macAddress();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  bool success = (httpCode == 200);
  
  if (success) {
    setSessionStatus(true, userUID);
  } else {
    apiConnected = false;
  }
  
  http.end();
  return success;
}

bool TimbangangMicroserviceClient::sendSessionEnd() {
  if (!apiConnected) return false;
  
  http.begin(API_SERVER_URL + "/session/end");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(128);
  doc["deviceId"] = WiFi.macAddress();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  bool success = (httpCode == 200);
  
  if (success) {
    setSessionStatus(false);
  } else {
    apiConnected = false;
  }
  
  http.end();
  return success;
}

void TimbangangMicroserviceClient::reconnectServices() {
  Serial.println("[API] Attempting to reconnect to services...");
  
  // Test API connection
  http.begin(API_SERVER_URL + "/status");
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    apiConnected = true;
    Serial.println("[API] Reconnected to API server");
  } else {
    apiConnected = false;
    Serial.println("[API] Failed to reconnect to API server");
  }
  
  http.end();
}

// Implementation of setter methods (same as original WebServer.cpp)
void TimbangangMicroserviceClient::setBaseMode(bool mode) {
  baseMode = mode;
}

void TimbangangMicroserviceClient::setBaseWeight(float weight) {
  baseWeight = weight;
}

void TimbangangMicroserviceClient::updateWeightData(WeightData data) {
  lastWeightData = data;
  
  // Calculate final weight with base correction
  if (baseMode && baseWeight > 0) {
    finalWeight = max(0.0f, data.filtered - baseWeight);
  } else {
    finalWeight = data.filtered;
  }
  
  // Send to microservice if connected
  if (apiConnected) {
    sendWeightData(data);
  }
}

void TimbangangMicroserviceClient::setRFIDStatus(String uid) {
  lastRFID = uid;
}

void TimbangangMicroserviceClient::setSystemReady(bool ready) {
  systemReady = ready;
}

void TimbangangMicroserviceClient::setStabilizationStatus(String status, int remainingTime) {
  stabilizationStatus = status;
  remainingStabilizationTime = remainingTime;
}

void TimbangangMicroserviceClient::setFinalWeight(float weight) {
  finalWeight = weight;
}

void TimbangangMicroserviceClient::setSessionStatus(bool active, String uid) {
  sessionActive = active;
  sessionUserUID = uid;
  if (active) {
    sessionStartTime = millis();
  } else {
    sessionStartTime = 0;
  }
}

String TimbangangMicroserviceClient::getWebServerIP() {
  return WiFi.localIP().toString();
}

// JSON response methods (simplified versions)
String TimbangangMicroserviceClient::getStatusJSON() {
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
  doc["apiConnected"] = apiConnected;
  doc["serverURL"] = LOAD_BALANCER_URL;
  
  String output;
  serializeJson(doc, output);
  return output;
}

String TimbangangMicroserviceClient::getConfigJSON() {
  DynamicJsonDocument doc(256);
  doc["baseMode"] = baseMode;
  doc["baseWeight"] = baseWeight;
  doc["serverURL"] = LOAD_BALANCER_URL;
  doc["apiConnected"] = apiConnected;
  
  String output;
  serializeJson(doc, output);
  return output;
}

// HTML pages (minimal versions to save flash memory)
String TimbangangMicroserviceClient::getMainPageHTML() {
  return R"HTML(<!DOCTYPE html><html><head><title>IoT Scale</title><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{font-family:Arial;margin:20px;text-align:center}.container{max-width:400px;margin:0 auto;padding:20px;border:1px solid #ddd;border-radius:10px}.weight{font-size:24px;font-weight:bold;color:#333;margin:10px 0}.access-status{padding:10px;margin:10px 0;border-radius:5px}.access-granted{background:#d4edda;color:#155724;border:1px solid #c3e6cb}.access-denied{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}.rfid-status{padding:8px;margin:5px 0;border-radius:5px;font-size:12px}.rfid-cached{background:#e2f3ff;color:#0066cc;border:1px solid #b3d9ff}.rfid-loading{background:#fff3cd;color:#856404;border:1px solid #ffeaa7}.btn{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:5px;margin:5px;cursor:pointer;display:block;width:90%}.btn:hover{background:#0056b3}</style></head><body><div class="container"><h1>IoT Scale</h1><div class="weight" id="weight">Loading...</div><div>Status: <span id="status">Connecting...</span></div><div id="access-status" class="access-status"></div><div id="rfid-status" class="rfid-status"></div><div>Server: <span>)HTML" + LOAD_BALANCER_URL + R"HTML(</span></div><button class="btn" onclick="location='/config'">Config</button><button class="btn" onclick="window.open(')HTML" + LOAD_BALANCER_URL + R"HTML(')">Dashboard</button></div><script>function update(){fetch('/api/status').then(r=>r.json()).then(d=>{document.getElementById('weight').textContent=d.weight.toFixed(3)+' kg';document.getElementById('status').textContent=d.apiConnected?'Connected':'Local';const accessDiv=document.getElementById('access-status');if(d.accessGranted){accessDiv.className='access-status access-granted';accessDiv.innerHTML='Access Granted<br>User: '+d.authorizedUser}else{accessDiv.className='access-status access-denied';accessDiv.innerHTML='Tap RFID for Access'}const rfidDiv=document.getElementById('rfid-status');if(d.rfidDataCached){rfidDiv.className='rfid-status rfid-cached';rfidDiv.innerHTML='📋 RFID Data: '+d.cachedUsers+' users cached'}else{rfidDiv.className='rfid-status rfid-loading';rfidDiv.innerHTML='⏳ Loading RFID data...'}}).catch(e=>console.error(e))}update();setInterval(update,3000)</script></body></html>)HTML";
}

String TimbangangMicroserviceClient::getConfigPageHTML() {
  return R"HTML(<!DOCTYPE html><html><head><title>Config</title><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{font-family:Arial;margin:20px}.form-group{margin:15px 0}label{display:block;margin-bottom:5px;font-weight:bold}input{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px}.btn{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:5px;margin:5px;cursor:pointer}.btn-danger{background:#dc3545}.btn-success{background:#28a745}</style></head><body><h1>Config</h1><div class="form-group"><label>Server URL:</label><input type="text" id="serverURL" placeholder="http://192.168.1.100"></div><div class="form-group"><label><input type="checkbox" id="baseMode"> Base Weight Mode</label></div><div class="form-group"><label>Base Weight (kg):</label><input type="number" id="baseWeight" step="0.001" placeholder="0.000"></div><button class="btn" onclick="save()">Save</button><button class="btn" onclick="cal()">Calibrate</button><button class="btn btn-success" onclick="syncRFID()">Sync RFID Data</button><button class="btn btn-danger" onclick="reset()">Reset</button><button class="btn" onclick="location='/'">Back</button><script>function load(){fetch('/api/config').then(r=>r.json()).then(d=>{document.getElementById('baseMode').checked=d.baseMode;document.getElementById('baseWeight').value=d.baseWeight;document.getElementById('serverURL').value=d.serverURL})}function save(){const c={baseMode:document.getElementById('baseMode').checked,baseWeight:parseFloat(document.getElementById('baseWeight').value)||0,serverURL:document.getElementById('serverURL').value};fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(c)}).then(r=>r.json()).then(d=>alert(d.message)).catch(e=>alert('Error:'+e))}function cal(){if(!confirm('Place base on scale. Continue?'))return;fetch('/api/calibrate',{method:'POST'}).then(r=>r.json()).then(d=>alert(d.message)).catch(e=>alert('Error:'+e))}function syncRFID(){if(!confirm('Sync RFID data from Firebase?'))return;fetch('/api/rfid-sync',{method:'POST'}).then(r=>r.json()).then(d=>alert(d.message+(d.cachedUsers?' - Users: '+d.cachedUsers:''))).catch(e=>alert('Error:'+e))}function reset(){if(!confirm('Reset config?'))return;fetch('/api/reset',{method:'POST'}).then(r=>r.json()).then(d=>{alert('Reset complete');load()}).catch(e=>alert('Error:'+e))}load()</script></body></html>)HTML";
}

// Configuration persistence methods (same as original)
void TimbangangMicroserviceClient::loadConfiguration() {
  EEPROM.begin(512);
  
  // Read base mode
  uint8_t mode = EEPROM.read(EEPROM_ADDR_BASE_MODE);
  if (mode == 0 || mode == 1) {
    baseMode = (mode == 1);
  }
  
  // Read base weight
  EEPROM.get(EEPROM_ADDR_BASE_WEIGHT, baseWeight);
  if (isnan(baseWeight) || baseWeight < 0 || baseWeight > 50) {
    baseWeight = DEFAULT_BASE_WEIGHT;
  }
  
  Serial.println("[CONFIG] Loaded - Base Mode: " + String(baseMode ? "ON" : "OFF") + 
                 ", Base Weight: " + String(baseWeight, 3) + " kg");
}

void TimbangangMicroserviceClient::saveConfiguration() {
  EEPROM.write(EEPROM_ADDR_BASE_MODE, baseMode ? 1 : 0);
  EEPROM.put(EEPROM_ADDR_BASE_WEIGHT, baseWeight);
  EEPROM.commit();
  
  Serial.println("[CONFIG] Saved - Base Mode: " + String(baseMode ? "ON" : "OFF") + 
                 ", Base Weight: " + String(baseWeight, 3) + " kg");
}

void TimbangangMicroserviceClient::finishBaseCalibration(float currentWeight) {
  if (currentWeight > 0.01) { // Minimum weight threshold
    baseWeight = currentWeight;
    baseMode = true;
    saveConfiguration();
    
    Serial.println("[CALIBRATION] Base calibrated: " + String(baseWeight, 3) + " kg");
  }
}
