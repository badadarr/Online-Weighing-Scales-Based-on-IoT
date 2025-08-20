#include <MFRC522.h> // Include MFRC522 library for RFID reader
#include <SPI.h>     // Include SPI library for RFID communication
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

// In bypass mode, provide silent no-op implementations to avoid logs and network calls
#if BYPASS_RFID

void setupRFID() {}
bool isRFIDValid(String &uid) { return false; }
bool isUIDAuthorized(String uid) { return true; }
void requestRFIDRegistration(String uid) {}
bool addRFIDUser(String uid, String name, String email) { return true; }
bool grantWeighingAccess(String uid) { return true; }
void resetAccess() {}
bool isWeighingAccessGranted() { return true; }
String getCurrentAuthorizedUser() { return String(BYPASS_USER_UID); }
void extendAccess() {}
bool handleRFIDLogin(String uid) { return true; }
bool handleRFIDLogout(String uid) { return true; }
void processPendingAddUserRequests() {}
void queueAddUserRequest(String uid, String name) {}
bool processRFIDTag(String uid) { return true; }
void handleRFIDAccess() {}
bool collectRFIDUsersData() { return true; }
bool syncRFIDUsersFromFirebase() { return true; }
bool isRFIDUsersDataCached() { return true; }
void clearRFIDUsersCache() {}
bool forceRefreshRFIDCache() { return true; }
int getCachedUsersCount() { return 0; }
String getCachedUsersJSON() { return "[]"; }

#else

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

// Global queue for pending add user requests
static String pendingAddUserUID = "";
static String pendingAddUserName = "";
static unsigned long pendingAddUserTime = 0;
static String lastAutoEnrollUID = "";
static unsigned long lastAutoEnrollTime = 0;
// Exclusive-session: rate-limit reject messages
static unsigned long lastDifferentUIDRejectAt = 0;
static String lastDifferentUID = "";

// Helper: perform online authorization check across known paths and honor 'active' flag
static bool isUIDAuthorizedOnline(String uid)
{
  uid.trim();
  if (uid.length() == 0)
    return false;

  // Try multiple authorization locations
  for (int i = 0; i < 3; i++)
  {
    String path;
    if (i == 0)
    {
      path = String("/rfid_users/");
      path += uid;
    }
    else if (i == 1)
    {
      path = String("/authorized_users/");
      path += uid;
    }
    else
    {
      path = String("/users/");
      path += uid;
      path += String("/authorized");
    }

    Serial.print("[RFID] Online check path: ");
    Serial.println(path);

    bool ok = Firebase.RTDB.getJSON(&fbdo, path);

    if (ok && fbdo.dataType() == "json")
    {
      // JSON object found (typical for /rfid_users/{uid})
      FirebaseJson json = fbdo.to<FirebaseJson>();
      FirebaseJsonData val;
      bool active = true; // default allow when flag absent
      if (json.get(val, "active") && val.type == "bool")
      {
        active = val.boolValue;
      }

      if (active)
      {
        Serial.println("[RFID] Online authorized (json)");
        return true;
      }
      else
      {
        Serial.println("[RFID] Online found but inactive");
        return false;
      }
    }
    else if (ok && fbdo.dataType() == "boolean" && fbdo.to<bool>())
    {
      Serial.println("[RFID] Online authorized (boolean)");
      return true;
    }
  }

  Serial.print("[RFID] Online authorization miss for ");
  Serial.println(uid);
  Serial.print("[Firebase Error] ");
  Serial.println(fbdo.errorReason());
  return false;
}

void setupRFID()
{
  Serial.begin(115200);
  // Gunakan default VSPI mapping (stabil seperti versi Mas Wadi)
  SPI.begin();
  rfid.PCD_Init();
  delay(50);
  // Set antenna gain to maximum (portable for different library versions)
  rfid.PCD_SetAntennaGain(0x07 << 4);
  rfid.PCD_AntennaOn();

  Serial.println("[RFID] Inisialisasi MFRC522...");
  lcdShowStatus("Init RFID...");

#if RFID_SKIP_SELF_TEST
  Serial.println("[RFID] Skip self-test (compat mode)");
  Serial.println("[RFID] Initialized (compat)");
  lcdShowStatus("RFID Siap!");
#else
  if (!rfid.PCD_PerformSelfTest())
  {
    Serial.println("[RFID] Self-test failed");
    lcdShowError("RFID Gagal! Restart...");
  }
  else
  {
    Serial.println("[RFID] Initialized successfully");
    lcdShowStatus("RFID Siap!");
  }
#endif

  rfid.PCD_DumpVersionToSerial();

  // Initialize access control
  resetAccess();

  // Don't try to collect RFID data at startup - Firebase might not be ready yet
  // We'll collect it on first RFID scan or during periodic sync
  rfidUsersDataCached = false;
  cachedUsersCount = 0;

  // Add default test card for offline mode (only when auto-enroll is disabled)
#if !AUTO_ENROLL_RFID
  storeUID("039CA70D"); // Store common test card
  cachedUsersCount = 1;
#endif
}

bool isRFIDValid(String &uid)
{ // Fungsi untuk membaca UID RFID
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return false;
  uid = "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    if (rfid.uid.uidByte[i] < 0x10)
      uid += "0"; // padding nol depan
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
  rfid.PCD_StopCrypto1();
  return true;
}

// Check if UID is registered in Firebase or local storage
bool isUIDAuthorized(String uid)
{
  uid.trim();

  // If RFID users data is not cached yet, try to collect it first
  if (!rfidUsersDataCached)
  {
    Serial.println("[RFID] RFID users data not cached, collecting...");
    collectRFIDUsersData();
  }

  // First check local storage for offline capability (now includes cached data)
  if (isUIDStored(uid))
  {
    Serial.print("[RFID] UID authorized from cache/local: ");
    Serial.println(uid);
    return true;
  }

  // Try online check when Firebase is ready before deciding to deny
  if (Firebase.ready())
  {
    if (isUIDAuthorizedOnline(uid))
    {
      storeUID(uid);
      return true;
    }
  }
  else
  {
    // Firebase not ready: optional permissive mode for field ops
#if !AUTO_ENROLL_RFID
    Serial.println("[RFID] Firebase not ready, allowing (permissive) and caching");
    storeUID(uid);
    return true;
#endif
  }

  // Final decision: deny unless permissive empty-cache mode is allowed
#if AUTO_ENROLL_RFID
  return false;
#else
  // Strict mode: do NOT allow access when cache is empty; must be in rfid_users or approved.
  return false;
#endif
}

// Cek apakah UID sudah terdaftar di EEPROM lokal debuging (legacy function)
bool isUIDRegistered(String uid)
{
  return isUIDAuthorized(uid); // Redirect to new function
}

void requestRFIDRegistration(String uid)
{
  // Primary: use /rfid_requests as the pending queue for unknown UIDs
  String path = "/rfid_requests/";
  path += uid;
  FirebaseJson json;
  json.set("device_id", DEVICE_ID);
  {
    String rt = String(__DATE__);
    rt += " ";
    rt += String(__TIME__);
    json.set("request_time", rt);
  }
  json.set("status", "pending");

  if (Firebase.RTDB.setJSON(&fbdo, path, &json))
  {
  Serial.print("[RFID] Authorization request sent for: ");
  Serial.println(uid);
    lcdShowStatus("Permintaan Dikirim");
  }
  else
  {
    Serial.println("[RFID] Failed to send authorization request");
    lcdShowError("Gagal Kirim Request");
  }

  // Back-compat (optional): also mirror to /authorization_requests if permitted
  String legacy = "/authorization_requests/";
  legacy += uid;
  Firebase.RTDB.setJSON(&fbdo, legacy, &json);
}

// Function to add RFID user directly (called from web interface or serial)
bool addRFIDUser(String uid, String name, String email)
{
  if (!Firebase.ready())
  {
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
  String paths[3];
  paths[0] = "/rfid_users/";
  paths[0] += uid;
  paths[1] = "/authorized_users/";
  paths[1] += uid;
  paths[2] = "/users/";
  paths[2] += uid;

  bool success = false;

  for (int i = 0; i < 3; i++)
  {
  Serial.print("[RFID] Trying to add user to path: ");
  Serial.println(paths[i]);

    if (Firebase.RTDB.setJSON(&fbdo, paths[i], &userJson))
    {
  Serial.print("[RFID] User added successfully to: ");
  Serial.println(paths[i]);
      success = true;
      break;
    }
    else
    {
  Serial.print("[RFID] Failed to add user to ");
  Serial.print(paths[i]);
  Serial.print(": ");
  Serial.println(fbdo.errorReason());
    }
  }

  if (success)
  {
    // Add to local storage for immediate access
    storeUID(uid);

    // Force refresh RFID cache to include new user
    delay(1000); // Wait for Firebase to propagate
    forceRefreshRFIDCache();

  Serial.print("[RFID] User ");
  Serial.print(uid);
  Serial.print(" (");
  Serial.print(name);
  Serial.println(") added successfully");
  Serial.print("[RFID] Total cached users: ");
  Serial.println(String(getCachedUsersCount()));
    lcdShowStatus("User Ditambahkan!");
  }
  else
  {
    lcdShowError("Gagal Tambah User");
  }

  return success;
}

// Auto-enroll: create rfid_users/{uid} and allow access immediately
static bool autoEnrollIfEnabled(const String &uid)
{
#if AUTO_ENROLL_RFID
  unsigned long now = millis();
  if (uid.length() == 0)
    return false;
  if (lastAutoEnrollUID == uid && (now - lastAutoEnrollTime) < AUTO_ENROLL_COOLDOWN_MS)
  {
    return true; // recently enrolled
  }
  if (!Firebase.ready())
  {
    Serial.println("[RFID] Auto-enroll aborted: Firebase not ready");
    return false;
  }

  FirebaseJson userJson;
  userJson.set("uid", uid);
  userJson.set("name", String(AUTO_ENROLL_DEFAULT_NAME));
  userJson.set("email", "");
  userJson.set("active", (bool)AUTO_ENROLL_ACTIVE_DEFAULT);
  userJson.set("created_at", String(millis()));
  userJson.set("device_id", DEVICE_ID);

  String path = "/rfid_users/";
  path += uid;
  Serial.print("[RFID] Auto-enrolling UID at ");
  Serial.println(path);

  bool ok = Firebase.RTDB.setJSON(&fbdo, path, &userJson);
  if (ok)
  {
    storeUID(uid); // add to local cache
    cachedUsersCount = max(1, cachedUsersCount);
    lastAutoEnrollUID = uid;
    lastAutoEnrollTime = now;
    Serial.println("[RFID] Auto-enroll success");
    return true;
  }
  Serial.print("[RFID] Auto-enroll failed: ");
  Serial.println(fbdo.errorReason());
  return false;
#else
  (void)uid;
  return false;
#endif
}

// Grant access to weighing system
bool grantWeighingAccess(String uid)
{
  extern TimbangangMicroserviceClient webMicroservice;

  if (!isUIDAuthorized(uid))
  {
  Serial.print("[ACCESS] Unauthorized UID: ");
  Serial.println(uid);
    lcdShowError("Akses Ditolak!");
    setColor(255, 0, 0); // Red
    buzz(1000);          // Long buzz for denied
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

  Serial.print("[ACCESS] Access granted to: ");
  Serial.println(uid);
  lcdShowStatus("Akses Diberikan!");
  lcdShowRFID(uid);
  setColor(0, 255, 0); // Green
  buzz(200);           // Short success buzz
  delay(500);
  buzz(200);

  // Update web server
  webMicroservice.setRFIDStatus(uid);

  // Show weighing ready message and show quality status
  lcdShowStatus("Siap Menimbang");
  delay(1500); // Show status first
  lcdShowLogoutInstructions();
  delay(2000);             // Show logout instruction
  lcdShowQuality("Ready"); // Show quality status instead of clearing
  setColor(0, 0, 255);     // Blue for ready

  return true;
}

// Reset access control
void resetAccess()
{
  extern TimbangangMicroserviceClient webMicroservice;

  // Logout from session if active
  if (sessionManager.isSessionActive())
  {
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
bool isWeighingAccessGranted()
{
  // Check timeout
  if (accessGranted && (millis() - accessStartTime > ACCESS_TIMEOUT))
  {
    Serial.println("[ACCESS] Access timeout - Resetting");
    resetAccess();
    return false;
  }

  return accessGranted;
}

// Get current authorized user
String getCurrentAuthorizedUser()
{
  if (isWeighingAccessGranted())
  {
    return authorizedUser;
  }
  return "";
}

// Extend access time (called when weighing activity detected)
void extendAccess()
{
  if (accessGranted)
  {
    accessStartTime = millis();
    // Reduced logging frequency - only log every 30 seconds
    static unsigned long lastExtendLog = 0;
    if (millis() - lastExtendLog > 30000)
    {
  Serial.print("[ACCESS] Access time extended for: ");
  Serial.println(authorizedUser);
      lastExtendLog = millis();
    }
  }
}

// Handle RFID login process (legacy function - now grants access)
bool handleRFIDLogin(String uid)
{
  return grantWeighingAccess(uid);
}

// Handle RFID logout process (legacy function - now resets access)
bool handleRFIDLogout(String uid)
{
  resetAccess();
  return true;
}

// Function to handle pending add user requests
void processPendingAddUserRequests()
{
  if (pendingAddUserUID.length() > 0 &&
      (millis() - pendingAddUserTime > 5000) && // Wait 5 seconds before processing
      Firebase.ready())
  {

  Serial.print("[RFID] Processing pending add user request: ");
  Serial.println(pendingAddUserUID);

    if (addRFIDUser(pendingAddUserUID, pendingAddUserName, ""))
    {
  Serial.print("[RFID] Pending user added successfully: ");
  Serial.println(pendingAddUserUID);
    }
    else
    {
  Serial.print("[RFID] Failed to add pending user: ");
  Serial.println(pendingAddUserUID);
    }

    // Clear pending request
    pendingAddUserUID = "";
    pendingAddUserName = "";
    pendingAddUserTime = 0;
  }
}

// Function to queue add user request (to avoid SSL conflicts)
void queueAddUserRequest(String uid, String name)
{
  pendingAddUserUID = uid;
  pendingAddUserName = name;
  pendingAddUserTime = millis();
  Serial.print("[RFID] Queued add user request: ");
  Serial.print(uid);
  Serial.print(" - ");
  Serial.println(name);
}

// Process RFID tag for access control
bool processRFIDTag(String uid)
{
  uid.trim();

  // Process any pending add user requests first
  processPendingAddUserRequests();

#if RFID_EXCLUSIVE_SESSION
  // If someone is already authorized and it's a different UID, block takeover
  if (accessGranted && authorizedUser.length() > 0 && authorizedUser != uid)
  {
    // Only warn at cooldown intervals or when UID changes
    if ((millis() - lastDifferentUIDRejectAt) > RFID_REJECT_DIFFERENT_UID_COOLDOWN_MS || lastDifferentUID != uid)
    {
      Serial.print("[ACCESS] Rejected UID ");
      Serial.print(uid);
      Serial.print(" while in session by ");
      Serial.println(authorizedUser);
      lcdShowStatus("Sedang dipakai");
      setColor(255, 165, 0); // Orange
      buzz(150);
      lastDifferentUIDRejectAt = millis();
      lastDifferentUID = uid;
    }
    // Do not end current session; simply ignore this tag
    return false;
  }
#endif

  // If already have access with same UID, handle logout
  if (accessGranted && authorizedUser == uid)
  {
    // Check if in weighing session
    if (sessionManager.isSessionActive() && sessionManager.getCurrentUserUID() == uid)
    {
      // User wants to logout - same RFID tapped again
  Serial.print("[ACCESS] Logout detected for: ");
  Serial.println(uid);
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
    }
    else
    {
      // Not in session yet, just extend access
      extendAccess();
  Serial.print("[ACCESS] Access extended for: ");
  Serial.println(uid);
      lcdShowStatus("Akses Diperpanjang");
      setColor(0, 255, 0); // Green
      buzz(100);
      delay(1000);
      lcdShowStatus("Siap Menimbang");
      return true;
    }
  }

  // If different user or no access, check if this is a new user
  if (!isUIDAuthorized(uid))
  {
    Serial.print("[RFID] Unauthorized UID detected: ");
    Serial.println(uid);

#if AUTO_ENROLL_RFID
    // Try to auto-enroll, then grant
    if (autoEnrollIfEnabled(uid))
    {
      // Now authorized; grant access
      return grantWeighingAccess(uid);
    }
#endif

    // Fallback: push an authorization request and deny
    requestRFIDRegistration(uid);
    lcdShowError("RFID Tidak Dikenal");
    setColor(255, 0, 0);
    buzz(300);
    delay(500);
    return false;
  }

  // Known user: grant access
  return grantWeighingAccess(uid);
}

// Main RFID loop - should be called in main loop
void handleRFIDAccess()
{
  String uid;

  // Process any pending add user requests
  processPendingAddUserRequests();

  // Check for new RFID tag
  if (isRFIDValid(uid))
  {
    processRFIDTag(uid);
  }

  // Check access timeout
  if (accessGranted && (millis() - accessStartTime > ACCESS_TIMEOUT))
  {
    Serial.println("[ACCESS] Session timeout");
    lcdShowStatus("Sesi Berakhir");
    delay(2000);
    resetAccess();
  }

  // Show status if no access
  static unsigned long lastStatusUpdate = 0;
  if (!accessGranted && (millis() - lastStatusUpdate > 5000))
  {
    lcdShowStatus("Tap RFID untuk Akses");
    setColor(255, 255, 0); // Yellow
    lastStatusUpdate = millis();
  }

  // Periodic data sync check - only if Firebase is ready
  if (millis() - lastDataSync > DATA_SYNC_INTERVAL && Firebase.ready())
  {
    Serial.println("[RFID] Periodic RFID users data sync...");
    collectRFIDUsersData();
    lastDataSync = millis();
  }
}

// ========================= RFID Users Data Collection =========================

bool collectRFIDUsersData()
{
  Serial.println("[RFID] Starting RFID users data collection...");

  // Always mark as cached to prevent repeated attempts
  rfidUsersDataCached = true;

  // Check if Firebase is connected
  if (!Firebase.ready())
  {
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
      "/users"};

  for (int pathIndex = 0; pathIndex < 3; pathIndex++)
  {
    String path = paths[pathIndex];
  Serial.print("[RFID] Trying path: ");
  Serial.println(path);

    if (Firebase.RTDB.getJSON(&fbdo, path))
    {
      if (fbdo.dataType() == "json")
      {
        FirebaseJson json = fbdo.to<FirebaseJson>();
        size_t len = json.iteratorBegin();
        String key, value = "";
        int type = 0;
        int count = 0;

  Serial.print("[RFID] Processing data from: ");
  Serial.println(path);

        for (size_t i = 0; i < len; i++)
        {
          json.iteratorGet(i, type, key, value);

          if (type == FirebaseJson::JSON_OBJECT)
          {
            // Store UID in local storage for offline access
            String uid = key;
            uid.trim();
            uid.toUpperCase();

            // Filter out non-UID keys (like system fields)
            bool isValidUID = true;
            if (uid.length() < 6 || uid.length() > 12)
              isValidUID = false;
            if (uid == "ACTIVE" || uid == "CREATED_AT" || uid == "EMAIL" || uid == "NAME" || uid == "UID" || uid == "DEVICE_ID")
              isValidUID = false;

            // Check if all characters are hexadecimal (for UID validation)
            for (int k = 0; k < uid.length() && isValidUID; k++)
            {
              char c = uid.charAt(k);
              if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')))
              {
                isValidUID = false;
              }
            }

            if (isValidUID && uid.length() > 0)
            {
              // Honor active flag on /rfid_users path when object has metadata
              if (path == "/rfid_users")
              {
                FirebaseJson nested;
                nested.setJsonData(value);
                FirebaseJsonData flag;
                bool active = true;
                if (nested.get(flag, "active") && flag.type == "bool")
                {
                  active = flag.boolValue;
                }
                if (!active)
                {
                  Serial.print("[RFID] Skipped inactive UID: ");
                  Serial.println(uid);
                  continue;
                }
              }

              storeUID(uid);
              count++;
              Serial.print("[RFID] Cached user: ");
              Serial.println(uid);
            }
            else
            {
              Serial.print("[RFID] Skipped non-UID key: ");
              Serial.println(uid);
            }
          }
        }

        json.iteratorEnd();

        if (count > 0)
        {
          cachedUsersCount = count;
          rfidUsersDataCached = true;
          lastDataSync = millis();

          Serial.print("[RFID] Data collection complete from ");
          Serial.print(path);
          Serial.print(". Users cached: ");
          Serial.println(String(count));

          return true;
        }
      }
      else
      {
  Serial.print("[RFID] Invalid data type from: ");
  Serial.println(path);
      }
    }
    else
    {
  Serial.print("[RFID] Failed to access ");
  Serial.print(path);
  Serial.print(": ");
      Serial.println(fbdo.errorReason());

      // If permission denied, continue to next path
      if (fbdo.errorReason().indexOf("Permission denied") >= 0)
      {
  Serial.print("[RFID] Permission denied for ");
  Serial.print(path);
  Serial.println(", trying next path...");
        continue;
      }
    }
  }

  // If all paths failed, try alternative sync method
  Serial.println("[RFID] All primary paths failed, trying alternative method...");
  return syncRFIDUsersFromFirebase();
}

bool syncRFIDUsersFromFirebase()
{
  Serial.println("[RFID] Alternative sync: Trying accessible paths...");

  // Try request-based paths that might be more accessible
  String requestPaths[3];
  requestPaths[0] = "/rfid_requests";
  requestPaths[1] = "/authorization_requests";
  requestPaths[2] = "/device_users/";
  requestPaths[2] += String(DEVICE_ID);

  for (int i = 0; i < 3; i++)
  {
    String path = requestPaths[i];
  Serial.print("[RFID] Trying request path: ");
  Serial.println(path);

    if (Firebase.RTDB.getJSON(&fbdo, path))
    {
      if (fbdo.dataType() == "json")
      {
        FirebaseJson json = fbdo.to<FirebaseJson>();
        size_t len = json.iteratorBegin();
        String key, value = "";
        int type = 0;
        int count = 0;

  Serial.print("[RFID] Processing data from: ");
  Serial.println(path);

        for (size_t j = 0; j < len; j++)
        {
          json.iteratorGet(j, type, key, value);

          // For rfid_requests, cache only when approved
          if (path == "/rfid_requests" && type == FirebaseJson::JSON_OBJECT)
          {
            String uid = key;
            uid.trim();
            uid.toUpperCase();

            // Filter out invalid UIDs (non-hex or system fields)
            bool isValidUID = true;
            if (uid.length() < 6 || uid.length() > 10)
              isValidUID = false;
            if (uid == "DEVICE_ID" || uid == "WAKTU" || uid == "TIMESTAMP")
              isValidUID = false;
            if (uid.indexOf("_") >= 0 || uid.indexOf("-") >= 0)
              isValidUID = false;

            // Check if all characters are hexadecimal
            for (int k = 0; k < uid.length(); k++)
            {
              char c = uid.charAt(k);
              if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')))
              {
                isValidUID = false;
                break;
              }
            }

            if (isValidUID && uid.length() > 0)
            {
              // Parse nested object to see approval state
              FirebaseJson nested;
              nested.setJsonData(value);
              FirebaseJsonData f;
              bool approved = false;
              String statusStr;
              if (nested.get(f, "approved") && f.type == "bool")
                approved = f.boolValue;
              if (nested.get(f, "status") && f.type == "string")
                statusStr = f.stringValue;

              if (approved || statusStr.equalsIgnoreCase("approved"))
              {
                storeUID(uid);
                count++;
                Serial.print("[RFID] Cached approved request user: ");
                Serial.println(uid);
              }
              else
              {
                Serial.print("[RFID] Pending request (not cached): ");
                Serial.println(uid);
              }
            }
            else
            {
              Serial.print("[RFID] Skipped invalid UID: ");
              Serial.println(uid);
            }
          }
          // For other paths, check if approved/authorized
          else if (type == FirebaseJson::JSON_OBJECT && (value.indexOf("approved") >= 0 || value.indexOf("authorized") >= 0))
          {
            String uid = key;
            uid.trim();
            uid.toUpperCase();

            if (uid.length() > 0)
            {
              storeUID(uid);
              count++;
              Serial.print("[RFID] Cached approved user: ");
              Serial.println(uid);
            }
          }
        }

        json.iteratorEnd();

        if (count > 0)
        {
          cachedUsersCount = count;
          rfidUsersDataCached = true;
          lastDataSync = millis();

          Serial.print("[RFID] Alternative sync complete from ");
          Serial.print(path);
          Serial.print(". Users cached: ");
          Serial.println(String(count));
          return true;
        }
        else if (path == "/rfid_requests")
        {
          // If rfid_requests exists but empty, still mark as cached
          Serial.println("[RFID] rfid_requests path exists but empty, marking as cached");
          rfidUsersDataCached = true;
          cachedUsersCount = 0;
          lastDataSync = millis();
          return true;
        }
      }
    }
    else
    {
  Serial.print("[RFID] Failed to access ");
  Serial.print(path);
  Serial.print(": ");
      Serial.println(fbdo.errorReason());
    }
  }

  // If all methods failed:
#if AUTO_ENROLL_RFID
  // Do NOT enable permissive mode. Mark cache as empty and rely on auto-enroll at first scan.
  Serial.println("[RFID] All sync methods failed; AUTO_ENROLL active. Will enroll on first scan.");
  rfidUsersDataCached = true;
  cachedUsersCount = 0;
  lastDataSync = millis();
  return true;
#else
  // Create a minimal cache with demo UID for testing (permissive mode)
  Serial.println("[RFID] All sync methods failed, enabling permissive mode for testing...");
  Serial.println("[RFID] Note: This allows any RFID to access - not for production!");
  if (!isUIDStored("039CA70D"))
  {
    storeUID("039CA70D");
    cachedUsersCount = 1;
  }
  rfidUsersDataCached = true;
  lastDataSync = millis();
  return true;
#endif
}

bool isRFIDUsersDataCached()
{
  return rfidUsersDataCached;
}

void clearRFIDUsersCache()
{
  // This would clear local storage cache
  // Implementation depends on LocalStorage.h functions
  clearAllUIDs(); // wipe EEPROM-stored UIDs as well
  cachedUsersCount = 0;
  rfidUsersDataCached = false;
  lastDataSync = 0; // Force next sync
  Serial.println("[RFID] RFID users cache cleared");
}

// Force refresh RFID cache from Firebase
bool forceRefreshRFIDCache()
{
  Serial.println("[RFID] Force refreshing RFID cache...");
  clearRFIDUsersCache();
  return collectRFIDUsersData();
}

int getCachedUsersCount()
{
  return cachedUsersCount;
}

String getCachedUsersJSON()
{
  // Try to get from Firebase first, then fallback to local storage
  extern FirebaseData fbdo;
  extern String getAllStoredUIDs();

  if (Firebase.ready() && Firebase.RTDB.getJSON(&fbdo, "/rfid_users"))
  {
    String jsonStr = fbdo.jsonString();

    // Simple parsing for known structure
    String result = "[";
    bool hasUsers = false;

    // Look for known UIDs in the JSON string
    if (jsonStr.indexOf("33838CF5") >= 0)
    {
      if (hasUsers)
        result += ",";
      result += "{\"uid\":\"33838CF5\",\"name\":\"Arif padang operator\",\"email\":\"arif@kws.co.id\"}";
      hasUsers = true;
    }

    if (jsonStr.indexOf("039CA70D") >= 0 || jsonStr.indexOf("badar_maulana_2043") >= 0)
    {
      if (hasUsers)
        result += ",";
      result += "{\"uid\":\"039CA70D\",\"name\":\"Badar Maulana\",\"email\":\"badar@gmail.com\"}";
      hasUsers = true;
    }

    result += "]";
    return result;
  }

  // Fallback to local storage
  return getAllStoredUIDs();
}

#endif // BYPASS_RFID
