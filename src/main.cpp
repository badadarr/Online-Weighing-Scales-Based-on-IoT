/**
 * @file main.cpp
 * @brief Program utama sistem timbangan IoT dengan kontrol akses RFID
 * @details Mengintegrasikan semua modul: sensor HX711, RFID MFRC522, LCD, 
 *          web server, Firebase, dan session management untuk sistem timbangan
 *          yang dapat diakses secara remote dengan kontrol akses berbasis RFID
 */

#include <Arduino.h>             // Arduino core library
#include <Firebase_ESP_Client.h> // Firebase ESP Client library
#include <WiFi.h>                // WiFi library untuk ESP32
#include <Wire.h>                // I2C communication library
#include <LiquidCrystal_I2C.h>   // LCD I2C display library

// ==================== CUSTOM LIBRARIES ====================
#include "config.h"              // Konfigurasi global sistem
#include "NetworkManager.h"      // Manajemen koneksi WiFi
#include "FirebaseClient.h"      // Client Firebase untuk cloud storage
#include "SensorReader.h"        // Pembaca sensor timbangan HX711
#include "TimeSync.h"            // Sinkronisasi waktu NTP
#include "RFIDReader.h"          // Pembaca RFID MFRC522
#include "Indicator.h"           // Kontrol LED dan buzzer
#include "LocalStorage.h"        // Penyimpanan lokal EEPROM
#include "lcd_display.h"         // Tampilan LCD I2C
#include "pinManager.h"          // Definisi pin GPIO
#include "WebServerIntegrated.h" // Web server terintegrasi
#include "SessionManager.h"      // Manajemen sesi pengguna

// File: src/main.cpp
unsigned long lastUpdate = 0;
String lastUID = "";
bool sendingActive = false; // Status pengiriman data (now controlled by session)

// Global instances
extern SessionManager sessionManager;
extern TimbangangWebServerIntegrated webServer;

// System calibration control
bool systemCalibrationRequested = false;
float systemCalibrationWeight = 0.0;
bool webTareRequested = false;
bool webStopRequested = false;

// Forward declaration for stabilization state
static bool lastStableState = false;

// NEW: Function declarations
bool isWeightStable(float weight);

/**
 * @brief Fungsi setup() - Inisialisasi sistem saat startup
 * @details Menginisialisasi semua komponen hardware dan software:
 *          - Serial communication untuk debugging
 *          - Indikator LED dan buzzer
 *          - LCD display untuk user interface
 *          - Koneksi WiFi dan sinkronisasi waktu
 *          - EEPROM untuk penyimpanan lokal
 *          - Sensor RFID dan timbangan
 *          - Firebase untuk cloud storage
 *          - Web server untuk remote access
 *          - Session manager untuk kontrol akses
 */
void setup()
{
  // Inisialisasi komunikasi serial untuk debugging
  Serial.begin(115200);
  Serial.println("[SYSTEM] Booting...");
  Serial.println("[INFO] Ketik 'help' untuk melihat perintah kalibrasi");

  // Inisialisasi pin
  setupIndicators();
  LEDBuzz(100); // LED dan buzzer menyala sebagai tanda booting
  buzz(100);    // Bunyi buzzer sebagai tanda mulai

  // Inisialisasi LCD
  setupLCD();
  lcdShowStatus("Booting System...");
  lcdShowStatus("Inisialisasi...");

  // Inisialisasi WiFi, NTP, EEPROM, dan Firebase
  lcdShowStatus("Inis WiFi...");
  connectWiFi();
  syncTime();
  initEEPROM();
  // RFID init (skipped in bypass mode)
#if BYPASS_RFID
  Serial.println("[RFID] BYPASS enabled. Skipping RFID init.");
  lcdShowStatus("RFID Bypass Mode");
#else
  setupRFID();     // Inisialisasi RFID reader
#endif
  setupSensor();   // Inisialisasi sensor
  setupFirebase(); // Inisialisasi server / firebase
  
  // Initialize session as inactive
  lcdShowStatus("Init Session...");
  sendingActive = false; // Start with sending inactive

  // NEW: Initialize web server for configuration
  lcdShowStatus("Init Web Server...");
  webServer.init();
  webServer.begin();
  webServer.setSystemReady(true);
  Serial.print("[WEB] Web server started at: http://");
  Serial.println(webServer.getWebServerIP());

  lcdShowStatus("Siap digunakan...");
  ulangiBuzzer();
  lcdClear();
}
/**
 * @brief Fungsi loop() - Loop utama sistem yang berjalan terus menerus
 * @details Menangani semua operasi real-time sistem:
 *          - Watchdog feeding untuk mencegah system reset
 *          - Kontrol akses RFID dan session management
 *          - Pembacaan dan filtering data sensor timbangan
 *          - Deteksi stabilitas dan pengiriman data ke Firebase
 *          - Update display LCD dan indikator visual/audio
 *          - Handling web server requests
 *          - Processing serial commands untuk kalibrasi
 *          - Base weight correction dan quality assessment
 */
void loop()
{
  // ==================== WATCHDOG MANAGEMENT ====================
  // Feed watchdog secara berkala untuk mencegah system timeout
  static unsigned long lastWatchdogFeed = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastWatchdogFeed > WATCHDOG_FEED_INTERVAL_MS) {
    yield(); // Feed the watchdog
    lastWatchdogFeed = currentTime;
  }

  // Add delay to prevent excessive loop frequency (configurable)
  static unsigned long lastLoop = 0;

  // Limit loop frequency based on performance mode
  if (currentTime - lastLoop < LOOP_DELAY_MS)
  {
    delay(TASK_YIELD_DELAY_MS); // Small delay to prevent CPU overload and feed watchdog
    return;
  }
  lastLoop = currentTime;

  // Handle RFID access control first (skip when bypass)
#if BYPASS_RFID
  // No RFID handling in bypass
#else
  handleRFIDAccess();
#endif
  
  // Check if weighing access is granted before proceeding (always granted in bypass)
#if BYPASS_RFID
  bool accessGranted = true;
#else
  bool accessGranted = isWeighingAccessGranted();
#endif
  if (!accessGranted) {
    // No access granted - show waiting message and skip weighing operations
    static unsigned long lastAccessMsg = 0;
    if (currentTime - lastAccessMsg > 5000) {
      lcdShowStatus("Tap RFID untuk Akses");
      setColor(255, 255, 0); // Yellow
      lastAccessMsg = currentTime;
    }
    
    // Handle web server even without access
    webServer.handleClient();
    delay(100);
    return;
  } else {
    // Show logout reminder periodically when in weighing session
    static unsigned long lastLogoutReminder = 0;
    if (sessionManager.isSessionActive() && 
        currentTime - lastLogoutReminder > 30000) { // Every 30 seconds
      static bool showReminder = false;
      if (showReminder) {
        lcdShowLogoutInstructions();
        delay(2000);
        lcdShowStatus("Siap Menimbang");
      }
      showReminder = !showReminder;
      lastLogoutReminder = currentTime;
    }
  }
  
  // Extend access time when weighing activity detected (skip in bypass)
#if BYPASS_RFID
  // No session extension needed
#else
  static unsigned long lastWeighingActivity = 0;
  if (currentTime - lastWeighingActivity > 10000) { // Every 10 seconds instead of 1 second
    extendAccess();
    lastWeighingActivity = currentTime;
  }
#endif

  // Handle web server requests (optimized with yield)
  webServer.handleClient();
  yield(); // Prevent watchdog timeout
  
  // Optional: Handle integrated server loop (less frequent)
  static unsigned long lastWebLoop = 0;
  if (currentTime - lastWebLoop > FIREBASE_SYNC_INTERVAL_MS) { // Every 15 seconds
    webServer.loop();
    lastWebLoop = currentTime;
    yield(); // Prevent watchdog timeout
  }
  
  // Check for access timeout (handled in handleRFIDAccess())
  // Access control is managed by RFID system
  
  // Handle web requests
  if (webTareRequested) {
    Serial.println("[WEB] Performing tare via web request");
    lcdShowStatus("Web Tare");
    performTare();
    webTareRequested = false;
  }
  
  if (webStopRequested) {
    Serial.println("[WEB] Resetting access via web request");
#if BYPASS_RFID
  // In bypass, don't reset session; just pause sending
  sendingActive = false;
  lastStableState = false;
  lcdShowStatus("Bypass: Stopped");
#else
  resetAccess();
  sendingActive = false;
  lastStableState = false;
  lcdShowStatus("Access Reset");
#endif
    webStopRequested = false;
  }
  
  if (systemCalibrationRequested) {
    Serial.println("\n=== WEB SYSTEM CALIBRATION ===");
    Serial.println("[WEB] Starting system calibration: " + String(systemCalibrationWeight, 3) + " kg");
    Serial.println("[WEB] Step 1: Ensure scale is empty and stable");
    lcdShowStatus("Web Calib: Empty");
    delay(3000);
    
    Serial.println("[WEB] Step 2: Performing tare...");
    lcdShowStatus("Web Calib: Tare");
    performTare();
    delay(2000);
    
    Serial.println("[WEB] Step 3: Place " + String(systemCalibrationWeight, 3) + " kg weight and wait...");
    lcdShowStatus("Place Weight");
    delay(5000);
    
    Serial.println("[WEB] Step 4: Starting calibration...");
    lcdShowStatus("Calibrating...");
    Kalibrasi(systemCalibrationWeight);
    
    Serial.println("[WEB] Step 5: Remove weight");
    lcdShowStatus("Remove Weight");
    delay(3000);
    
    Serial.println("=== WEB CALIBRATION COMPLETE ===");
    lcdShowStatus("Calib Complete");
    systemCalibrationRequested = false;
    delay(2000);
  }

  // Get advanced weight data from sensor (with configurable frequency)
  static unsigned long lastWeightRead = 0;
  WeightData weightData;

  // Read weight data based on performance mode interval
  if (currentTime - lastWeightRead >= WEIGHT_READ_INTERVAL_MS)
  {
    weightData = getAdvancedWeightData();
    lastWeightRead = currentTime;
    yield(); // Prevent watchdog timeout during sensor reading
  }
  else
  {
    // Use cached weight data
    weightData = webServer.getLastWeightData();
  }

  // Apply base correction based on web configuration
  float finalWeight = weightData.stable;
  WeightData correctedWeightData = weightData;

  if (webServer.getBaseMode())
  {
    // Apply base correction to both stable and filtered weights
    if (weightData.stable > 0)
    {
      finalWeight = weightData.stable - webServer.getBaseWeight();
      if (finalWeight < 0)
        finalWeight = 0;
    }

    if (weightData.filtered > 0)
    {
      correctedWeightData.filtered = weightData.filtered - webServer.getBaseWeight();
      if (correctedWeightData.filtered < 0)
        correctedWeightData.filtered = 0;
    }

    correctedWeightData.stable = finalWeight;

// Log base correction based on configuration
#if ENABLE_BASE_CORRECTION_LOG
    static float lastBaseCorrected = -1;
    if (abs(finalWeight - lastBaseCorrected) > LOG_CHANGE_THRESHOLD)
    {
      Serial.print("[WEIGHT] Base corrected: ");
      Serial.print(weightData.stable, 3);
      Serial.print(" - ");
      Serial.print(webServer.getBaseWeight(), 3);
      Serial.print(" = ");
      Serial.println(finalWeight, 3);
      lastBaseCorrected = finalWeight;
    }
#endif
  }

  String berat = String(finalWeight, WEIGHT_PRECISION);
  String beratDisplay = String(correctedWeightData.filtered, WEIGHT_PRECISION);

// Configurable logging frequency
#if ENABLE_DETAILED_LOGGING
  static float lastLoggedWeight = -1;
  static String lastLoggedQuality = "";
  static unsigned long lastLogTime = 0;

  bool shouldLog = (abs(finalWeight - lastLoggedWeight) > LOG_CHANGE_THRESHOLD) ||
                   (weightData.quality != lastLoggedQuality) ||
                   (currentTime - lastLogTime > 5000); // Log every 5 seconds max

  if (shouldLog)
  {
    Serial.print("[WEIGHT] Final weight: ");
    Serial.print(berat);
    Serial.print(" kg, Quality: ");
    Serial.println(weightData.quality);

    lastLoggedWeight = finalWeight;
    lastLoggedQuality = weightData.quality;
    lastLogTime = currentTime;
  }
#endif

  // Update web server with weight data (configurable frequency)
  static unsigned long lastWebUpdate = 0;
  if (currentTime - lastWebUpdate >= WEB_UPDATE_INTERVAL_MS)
  {
    webServer.updateWeightData(correctedWeightData); // Use corrected weight data
    webServer.setFinalWeight(finalWeight);           // Update berat final yang sudah dikoreksi
    lastWebUpdate = currentTime;
  }

  // Update LCD (configurable frequency)
  static unsigned long lastLCDUpdate = 0;
  if (currentTime - lastLCDUpdate >= LCD_UPDATE_INTERVAL_MS)
  {
    lcdShowBerat(beratDisplay); // Use corrected display weight
    
    // Show base mode status on LCD
    static unsigned long lastBaseModeDisplay = 0;
    if (currentTime - lastBaseModeDisplay >= 5000) { // Update every 5 seconds
      lcdShowBaseMode(webServer.getBaseMode(), webServer.getBaseWeight());
      lastBaseModeDisplay = currentTime;
    }
    
    lastLCDUpdate = currentTime;
  }

  // Sistem tunggu stabil sebelum kirim ke DB
  static bool lastStableState = false;
  static float lastStableWeight = 0;
  static unsigned long stableStartTime = 0;
  const unsigned long STABLE_DURATION_MS = 3000; // Tunggu 3 detik stabil
  const float MIN_WEIGHT = 0.2;                  // Minimum 200g (lebih tinggi untuk menghindari noise)
  const float WEIGHT_CHANGE_THRESHOLD = 0.02;    // 20g threshold (lebih sensitif)

  if (finalWeight > MIN_WEIGHT && correctedWeightData.isStable)
  {
    if (!lastStableState)
    {
      // Baru mulai stabil
      stableStartTime = currentTime;
      lastStableState = true;
      lastStableWeight = finalWeight;
      setColor(0, 255, 255); // Cyan - tunggu stabil
      lcdShowQuality("Waiting");
      webServer.setStabilizationStatus("waiting", 3);
      buzz(BUZZ_WAITING); // Long beep for waiting
      Serial.println("[WEIGHT] Mulai tunggu stabilisasi: " + berat + " kg");
    }
    else
    {
      // Sudah stabil, cek durasi dan perubahan
      unsigned long stableDuration = currentTime - stableStartTime;
      float weightChange = abs(finalWeight - lastStableWeight);

      if (weightChange > WEIGHT_CHANGE_THRESHOLD)
      {
        // Berat berubah, reset timer
        stableStartTime = currentTime;
        lastStableWeight = finalWeight;
        setColor(255, 165, 0); // Orange - reset tunggu
        lcdShowQuality("Change");
        webServer.setStabilizationStatus("waiting", 3);
        Serial.println("[WEIGHT] Reset timer, berat berubah: " + String(weightChange, 3) + " kg");
      }
      else if (stableDuration >= STABLE_DURATION_MS)
      {
  // Allow sending when bypassing even without a session
  if (
#if BYPASS_RFID
      true
#else
      (sessionManager.isSessionActive() && sendingActive)
#endif
  ) {
          // Sudah stabil cukup lama, kirim ke Firebase
          webServer.setStabilizationStatus("sending", 0);
          sendBeratKeFirebase(berat);
          setColor(0, 255, 0); // Hijau - berhasil kirim
          lcdShowQuality("Sent OK"); // Changed to show Quality instead of Status
          Serial.println("[FIREBASE] Data stabil terkirim: " + berat + " kg");
          buzz(BUZZ_SUCCESS); // Success confirmation
        } else {
          // Session not active, don't send data
          setColor(255, 255, 0); // Yellow - no session
          lcdShowQuality("No Session");
          Serial.println("[FIREBASE] Data tidak dikirim - Tidak ada session aktif");
        }

        // Reset untuk pengiriman berikutnya
        lastStableState = false;
        lastStableWeight = 0; // Reset berat stabil
        webServer.setStabilizationStatus("standby", 0);
        
        // Show success message longer and then show quality
        delay(2000); // Show "Sent OK" for 2 seconds
        // Show quality instead of clearing the line
        if (correctedWeightData.quality == "stable") {
          lcdShowQuality("Stable");
        } else if (correctedWeightData.quality == "good") {
          lcdShowQuality("Good");
        } else {
          lcdShowQuality("Ready");
        }
        
        // Add delay to prevent immediate re-triggering
        delay(1000); // Additional 1 second pause
      }
      else
      {
        // Masih dalam periode tunggu
        int remainingSeconds = (STABLE_DURATION_MS - stableDuration) / 1000 + 1;
        setColor(0, 255, 255); // Cyan - tunggu
        lcdShowQuality("Wait: " + String(remainingSeconds) + "s");
        webServer.setStabilizationStatus("waiting", remainingSeconds);
      }
    }
  }
  else
  {
    // Reset jika tidak stabil atau berat terlalu kecil
    if (lastStableState)
    {
      Serial.println("[WEIGHT] Stabilisasi dibatalkan - Weight: " + String(finalWeight, 3) + ", Stable: " + String(correctedWeightData.isStable));
      lastStableState = false;
      lastStableWeight = 0; // Reset berat stabil
      
      // Show quality status when going back to standby
      static unsigned long lastStandbyClear = 0;
      if (finalWeight < MIN_WEIGHT && currentTime - lastStandbyClear > 3000) {
        lcdShowQuality("Ready"); // Show quality status instead of clearing
        lastStandbyClear = currentTime;
      }
    }

    // Feedback visual dan audio berdasarkan kondisi
    static String lastQuality = "";
    static unsigned long lastBuzzTime = 0;
    static unsigned long lastFirebaseDisplayUpdate = 0; // Control Firebase display updates

    if (correctedWeightData.quality == "motion")
    {
      setColor(255, 255, 0); // Kuning - gerakan
      if (currentTime - lastFirebaseDisplayUpdate > 1000) { // Update every 1 second
        lcdShowQuality("Motion");
        lastFirebaseDisplayUpdate = currentTime;
      }
      webServer.setStabilizationStatus("motion", 0);

      // Buzz only when status changes or every 3 seconds
      if (lastQuality != "motion" || (currentTime - lastBuzzTime > 3000))
      {
        buzz(BUZZ_MOTION);
        lastBuzzTime = currentTime;
      }
    }
    else if (correctedWeightData.quality == "stabilizing")
    {
      setColor(255, 165, 0); // Orange - stabilisasi
      if (currentTime - lastFirebaseDisplayUpdate > 1000) { // Update every 1 second
        lcdShowQuality("Stabilizing");
        lastFirebaseDisplayUpdate = currentTime;
      }
      webServer.setStabilizationStatus("stabilizing", 0);

      // Buzz only when status changes
      if (lastQuality != "stabilizing")
      {
        buzz(BUZZ_STABILIZING);
        lastBuzzTime = currentTime;
      }
    }
    else if (correctedWeightData.quality == "error")
    {
      setColor(255, 0, 0); // Merah - error
      if (currentTime - lastFirebaseDisplayUpdate > 2000) { // Update every 2 seconds
        lcdShowQuality("Error");
        lastFirebaseDisplayUpdate = currentTime;
      }
      webServer.setStabilizationStatus("error", 0);

      // Buzz every 5 seconds for error
      if (lastQuality != "error" || (currentTime - lastBuzzTime > 5000))
      {
        buzz(BUZZ_ERROR);
        lastBuzzTime = currentTime;
      }
    }
    else
    {
      // Stable/standby mode - show quality status based on actual quality
      static unsigned long lastQualityUpdate = 0;
      if (currentTime - lastQualityUpdate > 2000) { // Update every 2 seconds
        setColor(0, 255, 0); // Green - stable quality
        
        // Show actual quality from sensor
        if (correctedWeightData.quality == "stable") {
          lcdShowQuality("Stable");
        } else if (correctedWeightData.quality == "good") {
          lcdShowQuality("Good");
        } else {
          lcdShowQuality("Ready");
        }
        
        webServer.setStabilizationStatus("standby", 0);
        lastQualityUpdate = currentTime;
      }
      // No buzz for standby
    }

    lastQuality = correctedWeightData.quality;
  }

  // Handle tare button (fixed frequency for responsiveness)
  static unsigned long lastTareCheck = 0;
  if (currentTime - lastTareCheck >= 50)
  { // Check every 50ms
    updateTareButton();
    lastTareCheck = currentTime;
  }
  /////////////////////////
  // RFID Access Control (handled in handleRFIDAccess() above)
  // Access is already validated, system can proceed with weighing
  
  // Get current authorized user
#if BYPASS_RFID
  String currentUser = String(BYPASS_USER_NAME) + " (Bypass)";
#else
  String currentUser = getCurrentAuthorizedUser();
#endif
  
  // Update web server with current user info
  webServer.setRFIDStatus(currentUser);

  // Only extend access time when weighing, don't send data continuously
#if BYPASS_RFID
  // Always allow sending during bypass
  sendingActive = true;
#else
  if (accessGranted && sessionManager.isSessionActive() && berat != "")
  { 
    // Set sending active only when session is active
    sendingActive = sessionManager.isSessionActive();
    extendAccess(); // Extend access time when activity detected
  } else {
    // Set sending inactive when no session
    sendingActive = false;
  }
#endif

  // Cek perintah dari Serial Monitor
  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd.startsWith("kalibrasi"))
    {
      int spasi = cmd.indexOf(' ');
      if (spasi > 0)
      {
        float beratKalibrasi = cmd.substring(spasi + 1).toFloat();
        if (beratKalibrasi > 0)
        {
          Serial.println("\n=== KALIBRASI TIMBANGAN ===");
          Serial.println("[SYSTEM] Mulai kalibrasi dengan berat: " + String(beratKalibrasi, 3) + " kg");
          Serial.println("[SYSTEM] Pastikan timbangan kosong, lalu ketik 'ok' dan tekan Enter...");

          // Wait for confirmation
          String confirm = "";
          while (confirm != "ok")
          {
            if (Serial.available())
            {
              confirm = Serial.readStringUntil('\n');
              confirm.trim();
              confirm.toLowerCase();
            }
            delay(100);
          }

          Serial.println("[SYSTEM] Melakukan tare...");
          lcdShowStatus("Kalibrasi: Tare");
          performTare();
          delay(2000);

          Serial.println("[SYSTEM] Letakkan beban " + String(beratKalibrasi, 3) + " kg, lalu ketik 'ok' dan tekan Enter...");
          lcdShowStatus("Letakkan Beban");

          confirm = "";
          while (confirm != "ok")
          {
            if (Serial.available())
            {
              confirm = Serial.readStringUntil('\n');
              confirm.trim();
              confirm.toLowerCase();
            }
            // Show current reading while waiting
            WeightData tempData = getAdvancedWeightData();
            Serial.println("[INFO] Pembacaan saat ini: " + String(tempData.filtered, 4) + " kg");
            delay(1000);
          }

          lcdShowStatus("Kalibrasi...");
          Kalibrasi(beratKalibrasi);
          lcdShowStatus("Kalibrasi Selesai");
          Serial.println("=== KALIBRASI SELESAI ===");
          Serial.println("[INFO] Angkat beban dari timbangan untuk melanjutkan...");
          Serial.println("[INFO] Ketik 'status' untuk melihat hasil kalibrasi");
          Serial.println("[INFO] Ketik 'test' untuk test akurasi");
          
          // Reset stabilization state to prevent continuous sending
          lastStableState = false;
          sendingActive = false;
          
          // Wait for weight to be removed
          delay(3000);
          lcdShowStatus("Angkat Beban");
          
          // Wait until weight is removed (< 0.1 kg)
          while (true) {
            WeightData checkData = getAdvancedWeightData();
            if (checkData.filtered < 0.1) {
              Serial.println("[INFO] Beban diangkat. Kalibrasi selesai!");
              lcdShowStatus("Siap Digunakan");
              break;
            }
            Serial.println("[INFO] Tunggu beban diangkat: " + String(checkData.filtered, 3) + " kg");
            delay(1000);
          }
        }
        else
        {
          Serial.println("[ERROR] Berat kalibrasi tidak valid!");
        }
      }
      else
      {
        Serial.println("[ERROR] Format: kalibrasi <berat>");
        Serial.println("[INFO] Contoh: kalibrasi 1.0");
      }
    }
    else if (cmd == "tare")
    {
      Serial.println("[SYSTEM] Melakukan tare manual...");
      lcdShowStatus("Tare Manual");
      performTare();
      Serial.println("[SYSTEM] Tare selesai");
    }
    else if (cmd == "status")
    {
      WeightData currentData = getAdvancedWeightData();
      Serial.println("\n=== STATUS SENSOR ===");
      Serial.println("Raw Weight: " + String(currentData.raw, 4) + " kg");
      Serial.println("Filtered Weight: " + String(currentData.filtered, 4) + " kg");
      Serial.println("Stable Weight: " + String(currentData.stable, 4) + " kg");
      Serial.println("Quality: " + currentData.quality);
      Serial.println("Is Stable: " + String(currentData.isStable ? "Yes" : "No"));
      Serial.println("Has Motion: " + String(currentData.hasMotion ? "Yes" : "No"));
      Serial.println("Faktor Kalibrasi: " + String(getFaktorKalibrasi(), 6));
      Serial.println("Base Mode: " + String(webServer.getBaseMode() ? "ON" : "OFF"));
      Serial.println("Base Weight: " + String(webServer.getBaseWeight(), 3) + " kg");
      Serial.println("Final Weight: " + String(finalWeight, 3) + " kg");
      Serial.println("RFID Active: " + String(sendingActive ? "Yes" : "No"));
      Serial.println("Session Active: " + String(sessionManager.isSessionActive() ? "Yes" : "No"));
      if (sessionManager.isSessionActive()) {
        Serial.println("Current User: " + sessionManager.getCurrentUserUID());
        Serial.println("Session Duration: " + String(sessionManager.getSessionDuration() / 1000) + " seconds");
      }
      Serial.println("===================");
    }
    else if (cmd == "test")
    {
      Serial.println("\n=== TEST PEMBACAAN ===");
      for (int i = 0; i < 10; i++)
      {
        WeightData testData = getAdvancedWeightData();
        Serial.println("Test " + String(i + 1) + ": " + String(testData.filtered, 4) + " kg (" + testData.quality + ")");
        delay(500);
      }
      Serial.println("=== TEST SELESAI ===");
    }
  else if (cmd == "help")
    {
      Serial.println("\n=== PERINTAH YANG TERSEDIA ===");
      Serial.println("kalibrasi <berat> - Kalibrasi dengan berat standar (contoh: kalibrasi 1.0)");
      Serial.println("tare              - Lakukan tare manual (reset ke nol)");
      Serial.println("status            - Tampilkan status lengkap sensor");
      Serial.println("test              - Test pembacaan 10x berturut-turut");
      Serial.println("stop              - Hentikan pengiriman data dan logout");
      Serial.println("session           - Tampilkan status session saat ini");
      Serial.println("adduser <uid> <name> [email] - Tambah user RFID baru");
      Serial.println("refresh/sync      - Refresh cache RFID dari Firebase");
      Serial.println("help              - Tampilkan bantuan ini");
      Serial.println("\n=== TIPS KALIBRASI ===");
      Serial.println("1. Pastikan timbangan stabil dan tidak bergetar");
      Serial.println("2. Gunakan beban standar yang akurat");
      Serial.println("3. Tunggu hingga pembacaan stabil sebelum konfirmasi");
      Serial.println("4. Setelah kalibrasi, angkat beban untuk melanjutkan");
      Serial.println("===============================");
    }
    else if (cmd == "stop")
    {
#if BYPASS_RFID
  sendingActive = false;
  lastStableState = false;
  Serial.println("[SYSTEM] BYPASS: Pengiriman dihentikan sementara");
  lcdShowStatus("Bypass: Stopped");
#else
  if (sessionManager.isSessionActive()) {
    sessionManager.logout();
  }
  sendingActive = false;
  lastStableState = false;
  Serial.println("[SYSTEM] Pengiriman data ke Firebase dihentikan");
  Serial.println("[INFO] Scan RFID untuk login kembali");
  lcdShowStatus("Logged Out");
#endif
    }
    else if (cmd == "session")
    {
      sessionManager.printSessionStatus();
    }
    else if (cmd == "refresh" || cmd == "sync")
    {
#if BYPASS_RFID
      Serial.println("[SYSTEM] Bypass active: skipping RFID cache refresh");
#else
      Serial.println("[SYSTEM] Refreshing RFID cache from Firebase...");
      if (forceRefreshRFIDCache()) {
        Serial.println("[SYSTEM] RFID cache refreshed successfully!");
        Serial.println("[SYSTEM] Cached users: " + String(getCachedUsersCount()));
      } else {
        Serial.println("[SYSTEM] Failed to refresh RFID cache!");
      }
#endif
    }
    else if (cmd.startsWith("adduser"))
    {
      // Format: adduser <uid> <name> [email]
      int firstSpace = cmd.indexOf(' ');
      if (firstSpace > 0) {
        String params = cmd.substring(firstSpace + 1);
        int secondSpace = params.indexOf(' ');
        
        if (secondSpace > 0) {
          String uid = params.substring(0, secondSpace);
          String remaining = params.substring(secondSpace + 1);
          int thirdSpace = remaining.indexOf(' ');
          
          String name, email;
          if (thirdSpace > 0) {
            name = remaining.substring(0, thirdSpace);
            email = remaining.substring(thirdSpace + 1);
          } else {
            name = remaining;
            email = "";
          }
          
          if (uid.length() >= 6 && name.length() > 0) {
            Serial.println("[SYSTEM] Adding RFID user: " + uid + " - " + name);
            if (addRFIDUser(uid, name, email)) {
              Serial.println("[SYSTEM] User added successfully!");
            } else {
              Serial.println("[SYSTEM] Failed to add user!");
            }
          } else {
            Serial.println("[ERROR] Invalid UID or name!");
          }
        } else {
          Serial.println("[ERROR] Format: adduser <uid> <name> [email]");
          Serial.println("[INFO] Example: adduser 12CCB463 \"John Doe\" john@example.com");
        }
      } else {
        Serial.println("[ERROR] Format: adduser <uid> <name> [email]");
        Serial.println("[INFO] Example: adduser 12CCB463 \"John Doe\" john@example.com");
      }
    }
    else if (cmd != "")
    {
      Serial.println("[ERROR] Perintah tidak dikenal: " + cmd);
      Serial.println("[INFO] Ketik 'help' untuk melihat perintah yang tersedia");
    }
  }
}

// NEW: Helper function to check weight stability
bool isWeightStable(float weight)
{
  static float lastWeight = 0;
  static int stableCount = 0;
  const float tolerance = 0.01; // 10g tolerance
  const int requiredStableReadings = 5;

  if (abs(weight - lastWeight) < tolerance)
  {
    stableCount++;
  }
  else
  {
    stableCount = 0;
  }

  lastWeight = weight;
  return stableCount >= requiredStableReadings;
}
