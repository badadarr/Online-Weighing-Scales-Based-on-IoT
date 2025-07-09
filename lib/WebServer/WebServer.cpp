#include "WebServer.h"
#include "pinManager.h"
#include "Indicator.h"
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "SensorReader.h"

TimbangangConfigServer webServer;

TimbangangConfigServer::TimbangangConfigServer() {
  server = new AsyncWebServer(WEB_SERVER_PORT);
  baseMode = DEFAULT_BASE_MODE;
  baseWeight = DEFAULT_BASE_WEIGHT;
  lastWeightData = {0.0, 0.0, 0.0, false, false, "stable", 0};
  finalWeight = 0.0;
  lastRFID = "";
  systemReady = false;
  stabilizationStatus = "standby";
  remainingStabilizationTime = 0;
}

void TimbangangConfigServer::init() {
  loadConfiguration();
  
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
  
  server->on("/api/config", HTTP_POST, [this](AsyncWebServerRequest *request) {},
    NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = String((char*)data).substring(0, len);
      DynamicJsonDocument doc(512);
      deserializeJson(doc, body);
      
      if (doc.containsKey("baseMode")) {
        setBaseMode(doc["baseMode"]);
      }
      if (doc.containsKey("baseWeight")) {
        setBaseWeight(doc["baseWeight"]);
      }
      
      saveConfiguration();
      request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Configuration saved successfully\"}");
    });
  
  server->on("/api/calibrate", HTTP_POST, [this](AsyncWebServerRequest *request) {
    // Validasi berat untuk kalibrasi (gunakan raw weight untuk kalibrasi)
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
    
    // Lakukan kalibrasi
    finishBaseCalibration(currentWeight);
    setBaseMode(true);
    
    Serial.println("[WEB] Base calibration via web: " + String(currentWeight, 3) + " kg -> " + String(baseWeight, 3) + " kg");
    
    request->send(200, "application/json", 
      "{\"status\":\"success\",\"baseWeight\":" + String(baseWeight, WEIGHT_PRECISION) + 
      ",\"message\":\"Base calibrated successfully\"}");
  });
  
  server->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
    baseMode = DEFAULT_BASE_MODE;
    baseWeight = DEFAULT_BASE_WEIGHT;
    saveConfiguration();
    request->send(200, "application/json", "{\"status\":\"success\"}");
  });
  
  server->on("/api/tare", HTTP_POST, [this](AsyncWebServerRequest *request) {
    extern bool webTareRequested;
    webTareRequested = true;
    request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Tare requested\"}");
  });
  
  server->on("/api/system-calibrate", HTTP_POST, [this](AsyncWebServerRequest *request) {},
    NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = String((char*)data).substring(0, len);
      DynamicJsonDocument doc(256);
      deserializeJson(doc, body);
      
      if (!doc.containsKey("weight")) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Weight parameter required\"}");
        return;
      }
      
      float calibWeight = doc["weight"];
      if (calibWeight <= 0) {
        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid weight value\"}");
        return;
      }
      
      startSystemCalibration(calibWeight);
      request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Calibration started\"}");
    });
  
  server->on("/api/stop-sending", HTTP_POST, [this](AsyncWebServerRequest *request) {
    stopDataSending();
    request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Data sending stopped\"}");
  });
  
  server->on("/system", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getSystemPageHTML());
  });
  
  server->onNotFound([this](AsyncWebServerRequest *request) {
    request->send(404, "text/html", "Not Found");
  });
  
  Serial.println("[WEB] Web server routes configured");
}

void TimbangangConfigServer::begin() {
  server->begin();
  Serial.println("[WEB] Web server started on port " + String(WEB_SERVER_PORT));
  Serial.println("[WEB] Access: http://" + WiFi.localIP().toString());
}

void TimbangangConfigServer::handleClient() {
  // AsyncWebServer handles clients automatically
}

String TimbangangConfigServer::getStatusJSON() {
  DynamicJsonDocument doc(512);
  doc["weight"] = lastWeightData.filtered; // Display corrected filtered weight
  doc["rawWeight"] = lastWeightData.filtered; // Show corrected weight for consistency
  doc["baseMode"] = baseMode;
  doc["baseWeight"] = baseWeight;
  doc["lastRFID"] = lastRFID;
  doc["systemReady"] = systemReady;
  doc["weightQuality"] = lastWeightData.quality;
  doc["isStable"] = lastWeightData.isStable;
  doc["hasMotion"] = lastWeightData.hasMotion;
  doc["stabilizationStatus"] = stabilizationStatus;
  doc["remainingTime"] = remainingStabilizationTime;
  doc["canCalibrate"] = (lastWeightData.filtered > 0.01 && lastWeightData.isStable);
  
  String output;
  serializeJson(doc, output);
  return output;
}

String TimbangangConfigServer::getConfigJSON() {
  DynamicJsonDocument doc(256);
  doc["baseMode"] = baseMode;
  doc["baseWeight"] = baseWeight;
  
  String output;
  serializeJson(doc, output);
  return output;
}

String TimbangangConfigServer::getMainPageHTML() {
  return "<!DOCTYPE html><html><head><title>Timbangan IoT</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>body{font-family:Arial;margin:20px;background:#f5f5f5}.card{background:white;border:1px solid #ddd;padding:20px;margin:10px 0;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}.weight{font-size:2.5em;font-weight:bold;color:#28a745;text-align:center}.status{padding:10px;border-radius:5px;margin:10px 0;text-align:center;font-weight:bold}.status-standby{background:#f8f9fa;color:#6c757d}.status-waiting{background:#d1ecf1;color:#0c5460}.status-sending{background:#d4edda;color:#155724}.status-motion{background:#fff3cd;color:#856404}.status-error{background:#f8d7da;color:#721c24}.btn{display:inline-block;padding:12px 24px;margin:5px;background:#007bff;color:white;text-decoration:none;border-radius:5px;border:none;cursor:pointer}.btn:hover{background:#0056b3}</style></head><body><h1>Timbangan IoT Dashboard</h1><div class=\"card\"><h3>Current Weight</h3><div class=\"weight\" id=\"weight\">0.000 kg</div><div id=\"quality\"></div></div><div class=\"card\"><h3>Stabilization Status</h3><div class=\"status\" id=\"stabilization\">Stand by</div></div><div class=\"card\"><h3>Base Mode</h3><div id=\"mode\">WITHOUT BASE</div></div><div class=\"card\"><a href=\"/config\" class=\"btn\">Configuration</a><a href=\"/system\" class=\"btn\">System Control</a></div><script>setInterval(async()=>{try{const res=await fetch('/api/status');const data=await res.json();document.getElementById('weight').innerHTML=data.weight.toFixed(3)+' kg';document.getElementById('mode').innerHTML=data.baseMode?'WITH BASE':'WITHOUT BASE';const qualityText=data.isStable?'Stable':data.hasMotion?'Motion':'Stabilizing';document.getElementById('quality').innerHTML='Quality: '+qualityText;const stabDiv=document.getElementById('stabilization');const status=data.stabilizationStatus;if(status==='waiting'){stabDiv.innerHTML='Waiting for stable: '+data.remainingTime+'s';stabDiv.className='status status-waiting';}else if(status==='sending'){stabDiv.innerHTML='Sending to database...';stabDiv.className='status status-sending';}else if(status==='motion'){stabDiv.innerHTML='Motion detected';stabDiv.className='status status-motion';}else if(status==='error'){stabDiv.innerHTML='Sensor error';stabDiv.className='status status-error';}else{stabDiv.innerHTML='Stand by';stabDiv.className='status status-standby';}}catch(e){}},1000);</script></body></html>";
}

String TimbangangConfigServer::getConfigPageHTML() {
  return "<!DOCTYPE html><html><head><title>Configuration</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>body{font-family:Arial;margin:20px;background:#f5f5f5}.card{background:white;border:1px solid #ddd;padding:20px;margin:10px 0;border-radius:8px}.form-group{margin:15px 0}.btn{padding:12px 24px;margin:5px;border:none;background:#007bff;color:white;border-radius:5px;cursor:pointer}.btn-secondary{background:#6c757d}.btn-disabled{background:#ccc;cursor:not-allowed}.btn:hover:not(.btn-disabled){opacity:0.9}input[type=number]{padding:8px;width:200px;border:1px solid #ddd;border-radius:3px}label{display:block;margin:5px 0}.status-info{background:#e7f3ff;border:1px solid #b3d9ff;padding:10px;border-radius:5px;margin:10px 0}</style></head><body><h1>Configuration</h1><div class=\"card\"><h3>Current Status</h3><div class=\"status-info\" id=\"statusInfo\">Loading...</div></div><div class=\"card\"><div class=\"form-group\"><label><input type=\"radio\" name=\"baseMode\" value=\"false\" id=\"baseModeOff\"> Without Base</label><label><input type=\"radio\" name=\"baseMode\" value=\"true\" id=\"baseModeOn\"> With Base</label></div><div class=\"form-group\"><label>Base Weight (kg):</label><input type=\"number\" id=\"baseWeight\" step=\"0.001\" min=\"0\" placeholder=\"0.000\"></div><button onclick=\"saveConfig()\" class=\"btn\">Save Configuration</button><button onclick=\"calibrateBase()\" class=\"btn btn-secondary\" id=\"calibrateBtn\">Calibrate Base</button><button onclick=\"resetConfig()\" class=\"btn\" style=\"background:#dc3545\">Reset to Default</button></div><a href=\"/\">Back to Dashboard</a><script>let statusData={};async function updateStatus(){try{const res=await fetch('/api/status');statusData=await res.json();const info=document.getElementById('statusInfo');const calibrateBtn=document.getElementById('calibrateBtn');info.innerHTML='Raw Weight: '+statusData.rawWeight.toFixed(3)+' kg<br>Quality: '+(statusData.isStable?'Stable':'Not Stable')+'<br>Base Weight: '+statusData.baseWeight.toFixed(3)+' kg';if(statusData.canCalibrate){calibrateBtn.className='btn btn-secondary';calibrateBtn.disabled=false;}else{calibrateBtn.className='btn btn-secondary btn-disabled';calibrateBtn.disabled=true;}}catch(e){}}async function saveConfig(){const baseMode=document.getElementById('baseModeOn').checked;const baseWeight=parseFloat(document.getElementById('baseWeight').value)||0;const response=await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({baseMode,baseWeight})});if(response.ok){alert('Configuration saved!');setTimeout(updateStatus,300);}else{alert('Failed to save configuration');}}async function calibrateBase(){if(!statusData.canCalibrate){alert('Cannot calibrate: Weight must be stable and > 0.01 kg');return;}const response=await fetch('/api/calibrate',{method:'POST'});const data=await response.json();if(data.status==='success'){document.getElementById('baseWeight').value=parseFloat(data.baseWeight).toFixed(3);document.getElementById('baseModeOn').checked=true;alert('Base calibrated: '+parseFloat(data.baseWeight).toFixed(3)+' kg. Base mode enabled.');updateStatus();}else{alert('Calibration failed: '+(data.message||'Unknown error'));}}async function resetConfig(){if(confirm('Reset to default settings (Without Base, Weight = 0)?')){const response=await fetch('/api/reset',{method:'POST'});if(response.ok){document.getElementById('baseModeOff').checked=true;document.getElementById('baseWeight').value='0.000';alert('Configuration reset to default!');setTimeout(updateStatus,300);}else{alert('Failed to reset configuration');}}}fetch('/api/config').then(r=>r.json()).then(data=>{document.getElementById(data.baseMode?'baseModeOn':'baseModeOff').checked=true;document.getElementById('baseWeight').value=parseFloat(data.baseWeight).toFixed(3);});updateStatus();setInterval(updateStatus,1000);</script></body></html>";
}

void TimbangangConfigServer::setBaseMode(bool mode) { baseMode = mode; }
void TimbangangConfigServer::setBaseWeight(float weight) { baseWeight = weight; }
void TimbangangConfigServer::updateWeightData(WeightData data) { 
  lastWeightData = data; 
}
void TimbangangConfigServer::setFinalWeight(float weight) {
  finalWeight = weight;
}
void TimbangangConfigServer::setRFIDStatus(String uid) { lastRFID = uid; }
void TimbangangConfigServer::setSystemReady(bool ready) { systemReady = ready; }
void TimbangangConfigServer::setStabilizationStatus(String status, int remainingTime) {
  stabilizationStatus = status;
  remainingStabilizationTime = remainingTime;
}

String TimbangangConfigServer::getWebServerIP() {
  return WiFi.localIP().toString();
}

void TimbangangConfigServer::saveConfiguration() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(EEPROM_ADDR_BASE_MODE, baseMode ? 1 : 0);
  EEPROM.put(EEPROM_ADDR_BASE_WEIGHT, baseWeight);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("[WEB] Configuration saved");
}

void TimbangangConfigServer::loadConfiguration() {
  EEPROM.begin(EEPROM_SIZE);
  uint8_t savedMode = EEPROM.read(EEPROM_ADDR_BASE_MODE);
  EEPROM.get(EEPROM_ADDR_BASE_WEIGHT, baseWeight);
  EEPROM.end();
  
  baseMode = (savedMode == 1);
  if (isnan(baseWeight) || baseWeight < 0) {
    baseWeight = DEFAULT_BASE_WEIGHT;
  }
  Serial.println("[WEB] Configuration loaded");
}

void TimbangangConfigServer::finishBaseCalibration(float weight) {
  baseWeight = weight;
  saveConfiguration();
  Serial.println("[WEB] Base calibrated: " + String(baseWeight, WEIGHT_PRECISION) + " kg");
}

void TimbangangConfigServer::startSystemCalibration(float weight) {
  extern bool systemCalibrationRequested;
  extern float systemCalibrationWeight;
  systemCalibrationRequested = true;
  systemCalibrationWeight = weight;
  Serial.println("[WEB] System calibration requested: " + String(weight, 3) + " kg");
}

void TimbangangConfigServer::stopDataSending() {
  extern bool webStopRequested;
  webStopRequested = true;
  Serial.println("[WEB] Data sending stop requested");
}



String TimbangangConfigServer::getSystemPageHTML() {
  return "<!DOCTYPE html><html><head><title>System Control</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><style>body{font-family:Arial;margin:20px;background:#f5f5f5}.card{background:white;border:1px solid #ddd;padding:20px;margin:10px 0;border-radius:8px}.form-group{margin:15px 0}.btn{padding:12px 24px;margin:5px;border:none;background:#007bff;color:white;border-radius:5px;cursor:pointer}.btn-danger{background:#dc3545}.btn-warning{background:#ffc107;color:#000}.btn-success{background:#28a745}.btn:hover{opacity:0.9}input[type=number]{padding:8px;width:150px;border:1px solid #ddd;border-radius:3px}.status-info{background:#e7f3ff;border:1px solid #b3d9ff;padding:10px;border-radius:5px;margin:10px 0}.alert{padding:10px;border-radius:5px;margin:10px 0}.alert-warning{background:#fff3cd;color:#856404}</style></head><body><h1>System Control</h1><div class=\"card\"><h3>Current Status</h3><div class=\"status-info\" id=\"systemStatus\">Loading...</div></div><div class=\"card\"><h3>System Calibration</h3><div class=\"alert alert-warning\"><strong>Warning:</strong> System calibration will recalibrate the load cell. Use accurate standard weight.</div><div class=\"form-group\"><label>Standard Weight (kg):</label><input type=\"number\" id=\"calibWeight\" step=\"0.001\" min=\"0.1\" max=\"10\" value=\"1.000\"></div><button onclick=\"startCalibration()\" class=\"btn btn-warning\">Start System Calibration</button></div><div class=\"card\"><h3>System Controls</h3><button onclick=\"performTare()\" class=\"btn btn-success\">Perform Tare</button><button onclick=\"stopSending()\" class=\"btn btn-danger\">Stop Data Sending</button></div><a href=\"/\">Back to Dashboard</a><script>async function updateSystemStatus(){try{const res=await fetch('/api/status');const data=await res.json();const status=document.getElementById('systemStatus');status.innerHTML='Current Weight: '+data.weight.toFixed(3)+' kg<br>Quality: '+data.weightQuality+'<br>Is Stable: '+(data.isStable?'Yes':'No')+'<br>System Ready: '+(data.systemReady?'Yes':'No');}catch(e){}}async function startCalibration(){const weight=parseFloat(document.getElementById('calibWeight').value);if(!weight||weight<=0){alert('Please enter valid weight');return;}if(!confirm('Start system calibration with '+weight.toFixed(3)+' kg?')){return;}try{const response=await fetch('/api/system-calibrate',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({weight:weight})});const data=await response.json();if(data.status==='success'){alert('Calibration started!');}else{alert('Calibration failed: '+(data.message||'Unknown error'));}}catch(e){alert('Error starting calibration');}}async function performTare(){try{const response=await fetch('/api/tare',{method:'POST'});const data=await response.json();if(data.status==='success'){alert('Tare completed');updateSystemStatus();}else{alert('Tare failed');}}catch(e){alert('Error performing tare');}}async function stopSending(){if(confirm('Stop data sending to Firebase?')){try{const response=await fetch('/api/stop-sending',{method:'POST'});const data=await response.json();if(data.status==='success'){alert('Data sending stopped');}else{alert('Failed to stop sending');}}catch(e){alert('Error stopping data sending');}}}updateSystemStatus();setInterval(updateSystemStatus,2000);</script></body></html>";
}

bool TimbangangConfigServer::authenticate() {
  return true;
}

bool TimbangangConfigServer::startBaseCalibration() {
  return true;
}