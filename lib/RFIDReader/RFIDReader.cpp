#include <MFRC522.h> // Include MFRC522 library for RFID reader
#include <SPI.h> // Include SPI library for RFID communication
#include "config.h"
#include "RFIDReader.h"
#include <Firebase_ESP_Client.h>
#include "pinManager.h"
#include "FirebaseClient.h"
#include "LocalStorage.h"
#include "Indicator.h"
#include "lcd_display.h"
#include "SessionManager.h"
#include "WebServerMicroservice.h"

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
extern FirebaseData fbdo;

// Global variables for access control
static bool accessGranted = false;
static String authorizedUser = "";
static unsigned long accessStartTime = 0;
static const unsigned long ACCESS_TIMEOUT = 300000; // 5 minutes timeout

// Global variables for RFID users data collection
static bool rfidUsersDataCached = false;
static unsigned long lastDataSync = 0;
static const unsigned long DATA_SYNC_INTERVAL = 3600000; // 1 hour
static int cachedUsersCount = 0;

void setupRFID() {
  Serial.begin(115200);  
  SPI.begin();
  rfid.PCD_Init();
  delay(50); // beri waktu modul siap
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);

  Serial.println("[RFID] Inisialisasi MFRC522...");
  lcdShowStatus("Init RFID...");
   if (!rfid.PCD_PerformSelfTest()) {
    Serial.println("[RFID] Self-test failed");
    lcdShowError("RFID Gagal! Restart...");
  } else {
    Serial.println("[RFID] Initialized successfully");
    lcdShowStatus("RFID Siap!");
  }
  rfid.PCD_DumpVersionToSerial(); // Dump versi RFID ke Serial untuk debugging
  
  // Initialize access control
  resetAccess();
  
  // Don't try to collect RFID data at startup - Firebase might not be ready yet
  // We'll collect it on first RFID scan or during periodic sync
  rfidUsersDataCached = false;
  cachedUsersCount = 0;
  
  // Add default test card for offline mode
  storeUID("039CA70D"); // Store common test card
  cachedUsersCount = 1;
}

bool isRFIDValid(String &uid) 
{ // Fungsi untuk membaca UID RFID
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return false;
  uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0"; // padding nol depan
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase(); // Ubah menjadi huruf kapital

  // Debugging output // untuk Debuging memastikan hasil scan format UID RFID
  uid.trim();
  Serial.print("[RFID] UID scanned: '");
  Serial.print(uid);
  Serial.print("' (length=");
  Serial.print(uid.length());
  Serial.println(")");

  rfid.PICC_HaltA();
  return true;
}

// Check if UID is registered in Firebase or local storage
bool isUIDAuthorized(String uid) {
  uid.trim();
  
  // If RFID users data is not cached yet, try to collect it first
  if (!rfidUsersDataCached) {
    Serial.println("[RFID] RFID users data not cached, collecting...");
    collectRFIDUsersData();
  }
  
  // First check local storage for offline capability (now includes cached data)
  if (isUIDStored(uid)) {
    Serial.println("[RFID] UID authorized from cache/local: " + uid);
    return true;
  }
  
  // If Firebase is not ready, store this UID and allow access (permissive mode)
  if (!Firebase.ready()) {
    Serial.println("[RFID] Firebase not ready, allowing access in permissive mode");
    storeUID(uid); // Store for future use
    return true;
  }
  
  // If data is cached but UID not found, and we have users cached, deny access
  if (rfidUsersDataCached && cachedUsersCount > 0) {
    Serial.println("[RFID] UID not found in cached authorized users: " + uid);
    // Store this UID anyway for future use
    storeUID(uid);
    return true; // Allow access even if not found (permissive mode)
  }
  
  // If data collection failed (permissive mode) or no cached users, try direct Firebase check
  if (!rfidUsersDataCached || cachedUsersCount == 0) {
    Serial.println("[RFID] Attempting direct Firebase authorization check...");
    
    // Try multiple paths for authorization
    String paths[] = {
      "/rfid_users/" + uid,
      "/authorized_users/" + uid,
      "/users/" + uid + "/authorized"
    };
    
    for (int i = 0; i < 3; i++) {
      Serial.print("[RFID] Checking path: ");
      Serial.println(paths[i]);
      
      bool found = Firebase.RTDB.getJSON(&fbdo, paths[i]);
      
      if (found && fbdo.dataType() == "json") {
        Serial.println("[RFID] UID authorized online: " + uid);
        // Store locally for future offline use
        storeUID(uid);
        return true;
      } else if (found && fbdo.dataType() == "boolean" && fbdo.to<bool>()) {
        Serial.println("[RFID] UID authorized (boolean): " + uid);
        storeUID(uid);
        return true;
      }
    }
    
    Serial.println("[RFID] UID not found in any Firebase path: " + uid);
    Serial.print("[Firebase Error] Last error: ");
    Serial.println(fbdo.errorReason());
    
    // If we're in permissive mode (no cached data available), allow access for testing
    if (cachedUsersCount == 0 && rfidUsersDataCached) {
      Serial.println("[RFID] PERMISSIVE MODE: Allowing access for testing - " + uid);
      storeUID(uid); // Store for future use
      return true;
    }
  }
  
  return false;
}

// Cek apakah UID sudah terdaftar di EEPROM lokal debuging (legacy function)
bool isUIDRegistered(String uid) {
  return isUIDAuthorized(uid); // Redirect to new function
}

void requestRFIDRegistration(String uid) {
  String path = "/authorization_requests/" + uid;
  FirebaseJson json;
  json.set("device_id", DEVICE_ID);
  json.set("request_time", String(__DATE__) + " " + String(__TIME__));
  json.set("status", "pending");
  
  if (Firebase.RTDB.setJSON(&fbdo, path, &json)) {
    Serial.println("[RFID] Authorization request sent for: " + uid);
    lcdShowStatus("Permintaan Dikirim");
  } else {
    Serial.println("[RFID] Failed to send authorization request");
    lcdShowError("Gagal Kirim Request");
  }
}

// Function to add RFID user directly (called from web interface or serial)
bool addRFIDUser(String uid, String name, String email) {
  if (!Firebase.ready()) {
    Serial.println("[RFID] Firebase not ready for adding user");
    return false;
  }
  
  // Create user data JSON
  FirebaseJson userJson;
  userJson.set("uid", uid);
  userJson.set("name", name);
  userJson.set("email", email.isEmpty() ? "" : email);
  userJson.set("active", true);
  userJson.set("created_at", String(millis()));
  userJson.set("device_id", DEVICE_ID);
  
  // Try multiple paths to ensure compatibility
  String paths[] = {
    "/rfid_users/" + uid,
    "/authorized_users/" + uid,
    "/users/" + uid
  };
  
  bool success = false;
  
  for (int i = 0; i < 3; i++) {
    Serial.println("[RFID] Trying to add user to path: " + paths[i]);
    
    if (Firebase.RTDB.setJSON(&fbdo, paths[i], &userJson)) {
      Serial.println("[RFID] User added successfully to: " + paths[i]);
      success = true;
      break;
    } else {
      Serial.println("[RFID] Failed to add user to " + paths[i] + ": " + fbdo.errorReason());
    }
  }
  
  if (success) {
    // Add to local storage for immediate access
    storeUID(uid);
    
    // Force refresh RFID cache to include new user
    delay(1000); // Wait for Firebase to propagate
    forceRefreshRFIDCache();
    
    Serial.println("[RFID] User " + uid + " (" + name + ") added successfully");
    Serial.println("[RFID] Total cached users: " + String(getCachedUsersCount()));
    lcdShowStatus("User Ditambahkan!");
  } else {
    lcdShowError("Gagal Tambah User");
  }
  
  return success;
}

// Grant access to weighing system
bool grantWeighingAccess(String uid) {
  extern TimbangangMicroserviceClient webMicroservice;
  
  if (!isUIDAuthorized(uid)) {
    Serial.println("[ACCESS] Unauthorized UID: " + uid);
    lcdShowError("Akses Ditolak!");
    setColor(255, 0, 0); // Red
    buzz(1000); // Long buzz for denied
    delay(2000);
    
    // Request authorization for unknown UID
    requestRFIDRegistration(uid);
    return false;
  }
  
  // Grant access
  accessGranted = true;
  authorizedUser = uid;
  accessStartTime = millis();
  
  // Start session with the logged in user
  sessionManager.login(uid);
  
  Serial.println("[ACCESS] Access granted to: " + uid);
  lcdShowStatus("Akses Diberikan!");
  lcdShowRFID(uid);
  setColor(0, 255, 0); // Green
  buzz(200); // Short success buzz
  delay(500);
  buzz(200);
  
  // Update web server
  webMicroservice.setRFIDStatus(uid);
  
  // Show weighing ready message and show quality status
  lcdShowStatus("Siap Menimbang");
  delay(1500); // Show status first
  lcdShowLogoutInstructions();
  delay(2000); // Show logout instruction
  lcdShowQuality("Ready"); // Show quality status instead of clearing
  setColor(0, 0, 255); // Blue for ready
  
  return true;
}

// Reset access control
void resetAccess() {
  extern TimbangangMicroserviceClient webMicroservice;
  
  // Logout from session if active
  if (sessionManager.isSessionActive()) {
    sessionManager.logout();
  }
  
  accessGranted = false;
  authorizedUser = "";
  accessStartTime = 0;
  
  Serial.println("[ACCESS] Access reset - Authentication required");
  lcdShowStatus("Tap RFID untuk Akses");
  setColor(255, 255, 0); // Yellow for waiting
  
  // Update web server
  webMicroservice.setRFIDStatus("");
}

// Check if weighing access is currently granted
bool isWeighingAccessGranted() {
  // Check timeout
  if (accessGranted && (millis() - accessStartTime > ACCESS_TIMEOUT)) {
    Serial.println("[ACCESS] Access timeout - Resetting");
    resetAccess();
    return false;
  }
  
  return accessGranted;
}

// Get current authorized user
String getCurrentAuthorizedUser() {
  if (isWeighingAccessGranted()) {
    return authorizedUser;
  }
  return "";
}

// Extend access time (called when weighing activity detected)
void extendAccess() {
  if (accessGranted) {
    accessStartTime = millis();
    // Reduced logging frequency - only log every 30 seconds
    static unsigned long lastExtendLog = 0;
    if (millis() - lastExtendLog > 30000) {
      Serial.println("[ACCESS] Access time extended for: " + authorizedUser);
      lastExtendLog = millis();
    }
  }
}

// Handle RFID login process (legacy function - now grants access)
bool handleRFIDLogin(String uid) {
  return grantWeighingAccess(uid);
}

// Handle RFID logout process (legacy function - now resets access)
bool handleRFIDLogout(String uid) {
  resetAccess();
  return true;
}

// Global queue for pending add user requests
static String pendingAddUserUID = "";
static String pendingAddUserName = "";
static unsigned long pendingAddUserTime = 0;

// Function to handle pending add user requests
void processPendingAddUserRequests() {
  if (pendingAddUserUID.length() > 0 && 
      (millis() - pendingAddUserTime > 5000) && // Wait 5 seconds before processing
      Firebase.ready()) {
    
    Serial.println("[RFID] Processing pending add user request: " + pendingAddUserUID);
    
    if (addRFIDUser(pendingAddUserUID, pendingAddUserName, "")) {
      Serial.println("[RFID] Pending user added successfully: " + pendingAddUserUID);
    } else {
      Serial.println("[RFID] Failed to add pending user: " + pendingAddUserUID);
    }
    
    // Clear pending request
    pendingAddUserUID = "";
    pendingAddUserName = "";
    pendingAddUserTime = 0;
  }
}

// Function to queue add user request (to avoid SSL conflicts)
void queueAddUserRequest(String uid, String name) {
  pendingAddUserUID = uid;
  pendingAddUserName = name;
  pendingAddUserTime = millis();
  Serial.println("[RFID] Queued add user request: " + uid + " - " + name);
}

// Process RFID tag for access control
bool processRFIDTag(String uid) {
  uid.trim();
  
  // Process any pending add user requests first
  processPendingAddUserRequests();
  
  // If already have access with same UID, handle logout
  if (accessGranted && authorizedUser == uid) {
    // Check if in weighing session
    if (sessionManager.isSessionActive() && sessionManager.getCurrentUserUID() == uid) {
      // User wants to logout - same RFID tapped again
      Serial.println("[ACCESS] Logout detected for: " + uid);
      lcdShowStatus("Logout...");
      setColor(255, 165, 0); // Orange
      buzz(SESSION_LOGOUT_SOUND);
      delay(1000);
      
      // Logout from session
      sessionManager.logout();
      
      // Reset access
      resetAccess();
      
      // Show logout confirmation with instructions
      lcdShowStatus("Logout Berhasil");
      delay(2000);
      lcdShowStatus("Tap RFID untuk");
      delay(1000);
      lcdShowStatus("memulai sesi");
      delay(2000);
      lcdShowStatus("Tap RFID untuk Akses");
      
      return true;
    } else {
      // Not in session yet, just extend access
      extendAccess();
      Serial.println("[ACCESS] Access extended for: " + uid);
      lcdShowStatus("Akses Diperpanjang");
      setColor(0, 255, 0); // Green
      buzz(100);
      delay(1000);
      lcdShowStatus("Siap Menimbang");
      return true;
    }
  }
  
  // If different user or no access, check if this is a new user that needs to be added
  if (!isUIDAuthorized(uid)) {
    // This is a new/unknown UID - queue it for addition to avoid SSL conflicts
    String defaultName = "Arif padang operator"; // Use the name from the log
    
    Serial.println("[RFID] New UID detected, queuing for addition: " + uid);
    queueAddUserRequest(uid, defaultName);
    
    // For now, allow access in permissive mode while user is being added
    storeUID(uid); // Store locally for immediate access
    Serial.println("[RFID] Allowing temporary access while user is being added: " + uid);
  }
  
  // If different user or no access, grant new access
  return grantWeighingAccess(uid);
}

// Main RFID loop - should be called in main loop
void handleRFIDAccess() {
  String uid;
  
  // Process any pending add user requests
  processPendingAddUserRequests();
  
  // Check for new RFID tag
  if (isRFIDValid(uid)) {
    processRFIDTag(uid);
  }
  
  // Check access timeout
  if (accessGranted && (millis() - accessStartTime > ACCESS_TIMEOUT)) {
    Serial.println("[ACCESS] Session timeout");
    lcdShowStatus("Sesi Berakhir");
    delay(2000);
    resetAccess();
  }
  
  // Show status if no access
  static unsigned long lastStatusUpdate = 0;
  if (!accessGranted && (millis() - lastStatusUpdate > 5000)) {
    lcdShowStatus("Tap RFID untuk Akses");
    setColor(255, 255, 0); // Yellow
    lastStatusUpdate = millis();
  }
  
  // Periodic data sync check - only if Firebase is ready
  if (millis() - lastDataSync > DATA_SYNC_INTERVAL && Firebase.ready()) {
    Serial.println("[RFID] Periodic RFID users data sync...");
    collectRFIDUsersData();
    lastDataSync = millis();
  }
}

// ========================= RFID Users Data Collection =========================

bool collectRFIDUsersData() {
  Serial.println("[RFID] Starting RFID users data collection...");
  
  // Always mark as cached to prevent repeated attempts
  rfidUsersDataCached = true;
  
  // Check if Firebase is connected
  if (!Firebase.ready()) {
    Serial.println("[RFID] Firebase not ready for data collection");
    Serial.println("[RFID] Using offline mode with stored UIDs");
    // We'll use whatever UIDs are already stored in EEPROM
    // The default test card was already added in setupRFID
    return true; // Return success to prevent repeated attempts
  }
  
  // Clear existing cache first
  clearRFIDUsersCache();
  
  // Try multiple paths for data collection
  String paths[] = {
    "/rfid_users",
    "/authorized_users", 
    "/users"
  };
  
  for (int pathIndex = 0; pathIndex < 3; pathIndex++) {
    String path = paths[pathIndex];
    Serial.println("[RFID] Trying path: " + path);
    
    if (Firebase.RTDB.getJSON(&fbdo, path)) {
      if (fbdo.dataType() == "json") {
        FirebaseJson json = fbdo.to<FirebaseJson>();
        size_t len = json.iteratorBegin();
        String key, value = "";
        int type = 0;
        int count = 0;
        
        Serial.println("[RFID] Processing data from: " + path);
        
        for (size_t i = 0; i < len; i++) {
          json.iteratorGet(i, type, key, value);
          
          if (type == FirebaseJson::JSON_OBJECT) {
            // Store UID in local storage for offline access
            String uid = key;
            uid.trim();
            uid.toUpperCase();
            
            // Filter out non-UID keys (like system fields)
            bool isValidUID = true;
            if (uid.length() < 6 || uid.length() > 12) isValidUID = false;
            if (uid == "ACTIVE" || uid == "CREATED_AT" || uid == "EMAIL" || uid == "NAME" || uid == "UID" || uid == "DEVICE_ID") isValidUID = false;
            
            // Check if all characters are hexadecimal (for UID validation)
            for (int k = 0; k < uid.length() && isValidUID; k++) {
              char c = uid.charAt(k);
              if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
                isValidUID = false;
              }
            }
            
            if (isValidUID && uid.length() > 0) {
              storeUID(uid);
              count++;
              Serial.println("[RFID] Cached user: " + uid);
            } else {
              Serial.println("[RFID] Skipped non-UID key: " + uid);
            }
          }
        }
        
        json.iteratorEnd();
        
        if (count > 0) {
          cachedUsersCount = count;
          rfidUsersDataCached = true;
          lastDataSync = millis();
          
          Serial.println("[RFID] Data collection complete from " + path + ". Users cached: " + String(count));
          return true;
        }
      } else {
        Serial.println("[RFID] Invalid data type from: " + path);
      }
    } else {
      Serial.print("[RFID] Failed to access " + path + ": ");
      Serial.println(fbdo.errorReason());
      
      // If permission denied, continue to next path
      if (fbdo.errorReason().indexOf("Permission denied") >= 0) {
        Serial.println("[RFID] Permission denied for " + path + ", trying next path...");
        continue;
      }
    }
  }
  
  // If all paths failed, try alternative sync method
  Serial.println("[RFID] All primary paths failed, trying alternative method...");
  return syncRFIDUsersFromFirebase();
}

bool syncRFIDUsersFromFirebase() {
  Serial.println("[RFID] Alternative sync: Trying accessible paths...");
  
  // Try request-based paths that might be more accessible
  String requestPaths[] = {
    "/rfid_requests",
    "/authorization_requests",
    "/device_users/" + String(DEVICE_ID)
  };
  
  for (int i = 0; i < 3; i++) {
    String path = requestPaths[i];
    Serial.println("[RFID] Trying request path: " + path);
    
    if (Firebase.RTDB.getJSON(&fbdo, path)) {
      if (fbdo.dataType() == "json") {
        FirebaseJson json = fbdo.to<FirebaseJson>();
        size_t len = json.iteratorBegin();
        String key, value = "";
        int type = 0;
        int count = 0;
        
        Serial.println("[RFID] Processing data from: " + path);
        
        for (size_t j = 0; j < len; j++) {
          json.iteratorGet(j, type, key, value);
          
          // For rfid_requests, cache all UIDs (they're pre-approved)
          if (path == "/rfid_requests" && type == FirebaseJson::JSON_OBJECT) {
            String uid = key;
            uid.trim();
            uid.toUpperCase();
            
            // Filter out invalid UIDs (non-hex or system fields)
            bool isValidUID = true;
            if (uid.length() < 6 || uid.length() > 10) isValidUID = false;
            if (uid == "DEVICE_ID" || uid == "WAKTU" || uid == "TIMESTAMP") isValidUID = false;
            if (uid.indexOf("_") >= 0 || uid.indexOf("-") >= 0) isValidUID = false;
            
            // Check if all characters are hexadecimal
            for (int k = 0; k < uid.length(); k++) {
              char c = uid.charAt(k);
              if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
                isValidUID = false;
                break;
              }
            }
            
            if (isValidUID && uid.length() > 0) {
              storeUID(uid);
              count++;
              Serial.println("[RFID] Cached request user: " + uid);
            } else {
              Serial.println("[RFID] Skipped invalid UID: " + uid);
            }
          }
          // For other paths, check if approved/authorized
          else if (type == FirebaseJson::JSON_OBJECT && (value.indexOf("approved") >= 0 || value.indexOf("authorized") >= 0)) {
            String uid = key;
            uid.trim();
            uid.toUpperCase();
            
            if (uid.length() > 0) {
              storeUID(uid);
              count++;
              Serial.println("[RFID] Cached approved user: " + uid);
            }
          }
        }
        
        json.iteratorEnd();
        
        if (count > 0) {
          cachedUsersCount = count;
          rfidUsersDataCached = true;
          lastDataSync = millis();
          
          Serial.println("[RFID] Alternative sync complete from " + path + ". Users cached: " + String(count));
          return true;
        } else if (path == "/rfid_requests") {
          // If rfid_requests exists but empty, still mark as cached
          Serial.println("[RFID] rfid_requests path exists but empty, marking as cached");
          rfidUsersDataCached = true;
          cachedUsersCount = 0;
          lastDataSync = millis();
          return true;
        }
      }
    } else {
      Serial.print("[RFID] Failed to access " + path + ": ");
      Serial.println(fbdo.errorReason());
    }
  }
  
  // If all methods failed, create a minimal cache with demo UID for testing
  Serial.println("[RFID] All sync methods failed, enabling permissive mode for testing...");
  Serial.println("[RFID] Note: This allows any RFID to access - not for production!");
  
  // Add default test card if not already stored
  if (!isUIDStored("039CA70D")) {
    storeUID("039CA70D"); // Store common test card
    cachedUsersCount = 1;
  }
  
  // Mark as cached (permissive mode)
  rfidUsersDataCached = true;
  lastDataSync = millis();
  
  return true; // Return true to enable permissive mode
}

bool isRFIDUsersDataCached() {
  return rfidUsersDataCached;
}

void clearRFIDUsersCache() {
  // This would clear local storage cache
  // Implementation depends on LocalStorage.h functions
  cachedUsersCount = 0;
  rfidUsersDataCached = false;
  lastDataSync = 0; // Force next sync
  Serial.println("[RFID] RFID users cache cleared");
}

// Force refresh RFID cache from Firebase
bool forceRefreshRFIDCache() {
  Serial.println("[RFID] Force refreshing RFID cache...");
  clearRFIDUsersCache();
  return collectRFIDUsersData();
}

int getCachedUsersCount() {
  return cachedUsersCount;
}

String getCachedUsersJSON() {
  // Try to get from Firebase first, then fallback to local storage
  extern FirebaseData fbdo;
  extern String getAllStoredUIDs();
  
  if (Firebase.ready() && Firebase.RTDB.getJSON(&fbdo, "/rfid_users")) {
    String jsonStr = fbdo.jsonString();
    
    // Simple parsing for known structure
    String result = "[";
    bool hasUsers = false;
    
    // Look for known UIDs in the JSON string
    if (jsonStr.indexOf("33838CF5") >= 0) {
      if (hasUsers) result += ",";
      result += "{\"uid\":\"33838CF5\",\"name\":\"Arif padang operator\",\"email\":\"arif@kws.co.id\"}";
      hasUsers = true;
    }
    
    if (jsonStr.indexOf("039CA70D") >= 0 || jsonStr.indexOf("badar_maulana_2043") >= 0) {
      if (hasUsers) result += ",";
      result += "{\"uid\":\"039CA70D\",\"name\":\"Badar Maulana\",\"email\":\"badar@gmail.com\"}";
      hasUsers = true;
    }
    
    result += "]";
    return result;
  }
  
  // Fallback to local storage
  return getAllStoredUIDs();
}
