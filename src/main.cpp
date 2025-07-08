#include <Arduino.h> // Include Arduino core library
#include <Firebase_ESP_Client.h> // Include Firebase ESP Client library
#include <WiFi.h> // Include WiFi library for ESP32
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
///////////////////////Self Library//////////////////////////////////////
#include "config.h" // Include configuration file
#include "NetworkManager.h" // Network manager untuk mengatur koneksi WiFi
#include "FirebaseClient.h" // Firebase client untuk mengatur koneksi ke Firebase
#include "SensorReader.h" // Sensor reader untuk membaca data dari sensor
#include "TimeSync.h" // Time sync untuk menyinkronkan waktu
#include "RFIDReader.h" // RFID reader untuk membaca data dari RFID
#include "Indicator.h" // Indicator untuk mengatur LED indikator
#include "LocalStorage.h" // Local storage untuk menyimpan data secara lokal
#include "lcd_display.h" // LCD display untuk menampilkan informasi
#include "pinManager.h" // Pin manager untuk mengatur pin GPIO
#include "WebServer.h" // NEW: Web server untuk konfigurasi via website

// File: src/main.cpp
unsigned long lastUpdate = 0;
String lastUID = "";
bool sendingActive = false; // Status pengiriman data

// NEW: Function declarations
bool isWeightStable(float weight);

void setup() {
  Serial.begin(115200);
  Serial.println("[SYSTEM] Booting...");

  // Inisialisasi pin
  setupIndicators();
  LEDBuzz(100); // LED dan buzzer menyala sebagai tanda booting
  buzz(100); // Bunyi buzzer sebagai tanda mulai

  // Inisialisasi LCD
  setupLCD();
  lcdShowStatus("Booting System...");
  lcdShowStatus("Inisialisasi...");

  // Inisialisasi WiFi, NTP, EEPROM, dan Firebase
  lcdShowStatus("Inis WiFi...");
  connectWiFi();
  syncTime();
  initEEPROM();
  setupRFID(); // Inisialisasi RFID reader
  setupSensor();//Inisialisasi sensor
  setupFirebase();//Inisialisasi server / firebase
  
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
void loop() {
  // Add delay to prevent excessive loop frequency (configurable)
  static unsigned long lastLoop = 0;
  unsigned long currentTime = millis();
  
  // Limit loop frequency based on performance mode
  if (currentTime - lastLoop < LOOP_DELAY_MS) {
    delay(10); // Small delay to prevent CPU overload
    return;
  }
  lastLoop = currentTime;
  
  // Handle web server requests
  webServer.handleClient();

  // Get advanced weight data from sensor (with configurable frequency)
  static unsigned long lastWeightRead = 0;
  WeightData weightData;
  
  // Read weight data based on performance mode interval
  if (currentTime - lastWeightRead >= WEIGHT_READ_INTERVAL_MS) {
    weightData = getAdvancedWeightData();
    lastWeightRead = currentTime;
  } else {
    // Use cached weight data
    weightData = webServer.getLastWeightData();
  }
  
  // Apply base correction based on web configuration
  float finalWeight = weightData.stable;
  if (webServer.getBaseMode() && finalWeight > 0) {
    finalWeight = finalWeight - webServer.getBaseWeight();
    if (finalWeight < 0) finalWeight = 0;
    
    // Log base correction based on configuration
    #if ENABLE_BASE_CORRECTION_LOG
    static float lastBaseCorrected = -1;
    if (abs(finalWeight - lastBaseCorrected) > LOG_CHANGE_THRESHOLD) {
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
  
  // Configurable logging frequency
  #if ENABLE_DETAILED_LOGGING
  static float lastLoggedWeight = -1;
  static String lastLoggedQuality = "";
  static unsigned long lastLogTime = 0;
  
  bool shouldLog = (abs(finalWeight - lastLoggedWeight) > LOG_CHANGE_THRESHOLD) || 
                   (weightData.quality != lastLoggedQuality) ||
                   (currentTime - lastLogTime > 5000); // Log every 5 seconds max
  
  if (shouldLog) {
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
  if (currentTime - lastWebUpdate >= WEB_UPDATE_INTERVAL_MS) {
    webServer.updateWeightData(weightData);
    webServer.setFinalWeight(finalWeight); // Update berat final yang sudah dikoreksi
    lastWebUpdate = currentTime;
  }
  
  // Update LCD (configurable frequency)
  static unsigned long lastLCDUpdate = 0;
  if (currentTime - lastLCDUpdate >= LCD_UPDATE_INTERVAL_MS) {
    lcdShowBerat(berat);
    lastLCDUpdate = currentTime;
  }
  
  // Sistem tunggu stabil sebelum kirim ke DB
  static bool lastStableState = false;
  static float lastStableWeight = 0;
  static unsigned long stableStartTime = 0;
  const unsigned long STABLE_DURATION_MS = 3000; // Tunggu 3 detik stabil
  const float MIN_WEIGHT = 0.1; // Minimum 100g
  const float WEIGHT_CHANGE_THRESHOLD = 0.05; // 50g threshold
  
  if (finalWeight > MIN_WEIGHT && weightData.isStable) {
    if (!lastStableState) {
      // Baru mulai stabil
      stableStartTime = currentTime;
      lastStableState = true;
      lastStableWeight = finalWeight;
      setColor(0, 255, 255); // Cyan - tunggu stabil
      lcdShowFirebase("Menunggu stabil...");
      webServer.setStabilizationStatus("waiting", 3);
      buzz(BUZZ_WAITING); // Long beep for waiting
      Serial.println("[WEIGHT] Mulai tunggu stabilisasi: " + berat + " kg");
    } else {
      // Sudah stabil, cek durasi dan perubahan
      unsigned long stableDuration = currentTime - stableStartTime;
      float weightChange = abs(finalWeight - lastStableWeight);
      
      if (weightChange > WEIGHT_CHANGE_THRESHOLD) {
        // Berat berubah, reset timer
        stableStartTime = currentTime;
        lastStableWeight = finalWeight;
        setColor(255, 165, 0); // Orange - reset tunggu
        lcdShowFirebase("Reset tunggu...");
        webServer.setStabilizationStatus("waiting", 3);
        Serial.println("[WEIGHT] Reset timer, berat berubah: " + berat + " kg");
      } else if (stableDuration >= STABLE_DURATION_MS) {
        // Sudah stabil cukup lama, kirim ke Firebase
        webServer.setStabilizationStatus("sending", 0);
        sendBeratKeFirebase(berat);
        setColor(0, 255, 0); // Hijau - berhasil kirim
        lcdShowFirebase("Data terkirim!");
        Serial.println("[FIREBASE] Data stabil terkirim: " + berat + " kg");
        buzz(BUZZ_SUCCESS); // Success confirmation
        
        // Reset untuk pengiriman berikutnya
        lastStableState = false;
        webServer.setStabilizationStatus("standby", 0);
        delay(1000);
      } else {
        // Masih dalam periode tunggu
        int remainingSeconds = (STABLE_DURATION_MS - stableDuration) / 1000 + 1;
        setColor(0, 255, 255); // Cyan - tunggu
        lcdShowFirebase("Tunggu " + String(remainingSeconds) + "s");
        webServer.setStabilizationStatus("waiting", remainingSeconds);
      }
    }
  } else {
    // Reset jika tidak stabil atau berat terlalu kecil
    if (lastStableState) {
      Serial.println("[WEIGHT] Stabilisasi dibatalkan");
      lastStableState = false;
    }
    
    // Feedback visual dan audio berdasarkan kondisi
    static String lastQuality = "";
    static unsigned long lastBuzzTime = 0;
    
    if (weightData.quality == "motion") {
      setColor(255, 255, 0); // Kuning - gerakan
      lcdShowFirebase("Gerakan terdeteksi");
      webServer.setStabilizationStatus("motion", 0);
      
      // Buzz only when status changes or every 3 seconds
      if (lastQuality != "motion" || (currentTime - lastBuzzTime > 3000)) {
        buzz(BUZZ_MOTION);
        lastBuzzTime = currentTime;
      }
    } else if (weightData.quality == "stabilizing") {
      setColor(255, 165, 0); // Orange - stabilisasi
      lcdShowFirebase("Stabilisasi...");
      webServer.setStabilizationStatus("stabilizing", 0);
      
      // Buzz only when status changes
      if (lastQuality != "stabilizing") {
        buzz(BUZZ_STABILIZING);
        lastBuzzTime = currentTime;
      }
    } else if (weightData.quality == "error") {
      setColor(255, 0, 0); // Merah - error
      lcdShowFirebase("Error sensor");
      webServer.setStabilizationStatus("error", 0);
      
      // Buzz every 5 seconds for error
      if (lastQuality != "error" || (currentTime - lastBuzzTime > 5000)) {
        buzz(BUZZ_ERROR);
        lastBuzzTime = currentTime;
      }
    } else {
      setColor(0, 0, 0); // Mati - standby
      lcdShowFirebase("Stand by...");
      webServer.setStabilizationStatus("standby", 0);
      // No buzz for standby
    }
    
    lastQuality = weightData.quality;
  }
  
  // Handle tare button (fixed frequency for responsiveness)
  static unsigned long lastTareCheck = 0;
  if (currentTime - lastTareCheck >= 50) { // Check every 50ms
    updateTareButton();
    lastTareCheck = currentTime;
  }
  /////////////////////////
  String uid;
  if (isRFIDValid(uid)) 
    { // Cek apakah RFID valid
    uid.trim();
    if (isUIDRegistered(uid) || isUIDStored(uid)) 
    { // Jika UID terdaftar atau disimpan
      // Toggle logic
      if (sendingActive && uid == lastUID) 
      { // dan Jika pengiriman aktif dan UID sama dengan yang terakhir
        sendingActive = false; // Matikan pengiriman
        lcdShowStatus("Stop Kirim");
        setColor(255, 0, 0);
        buzz(300);
        delay(500);
        Serial.print("[RFID] UID sudah aktif: ");
        Serial.println(uid);
        setColor(255, 255, 0);
        lastUID = "";
        delay(500);
        return;
      } else {
        sendingActive = true; // Aktifkan pengiriman
        lastUID = uid;
        Serial.print("[RFID] UID Terdeteksi: ");
        Serial.println(uid);
        
        // NEW: Update web server dengan RFID status
        webServer.setRFIDStatus(uid);
        
        lcdShowStatus("RFID OK");
        setColor(0, 255, 0); // Hijau
        buzz(100);        
        setColor(0, 255, 0); // Hijau
        delay(100);
        lcdShowRFID(uid);
        storeUID(uid);
        delay(500); // Tunda untuk menghindari pembacaan ganda
      }
    } else {
      Serial.println("[RFID] Tidak dikenal!"); // UID tidak terdaftar
      lcdShowError("RFID Tidak Dikenal");
      setColor(255, 0, 0);
      buzz(300);
      lcdShowStatus("RFID Baru");
      requestRFIDRegistration(uid);
      lcdShowStatus("Scan RFID...");    
      setColor(50, 255, 50);
      delay(2000);
    }
    delay(3000);
  }

  // Kirim berat jika aktif
  if (sendingActive && berat != "") { // Jika pengiriman aktif dan berat valid
    sendBeratKeFirebase(berat);
    delay(200); // Atur interval pengiriman sesuai kebutuhan
  }

  // Cek perintah dari Serial Monitor
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("kalibrasi")) {
      int spasi = cmd.indexOf(' ');
      if (spasi > 0) {
        float beratKalibrasi = cmd.substring(spasi + 1).toFloat();
        if (beratKalibrasi > 0) {
          Kalibrasi(beratKalibrasi);
        } else {
          Serial.println("[SYSTEM] Berat kalibrasi tidak valid!");
        }
      } else {
        Serial.println("[SYSTEM] Format: kalibrasi <berat>");
      }
    }
  }
}

// NEW: Helper function to check weight stability
bool isWeightStable(float weight) {
  static float lastWeight = 0;
  static int stableCount = 0;
  const float tolerance = 0.01; // 10g tolerance
  const int requiredStableReadings = 5;
  
  if (abs(weight - lastWeight) < tolerance) {
    stableCount++;
  } else {
    stableCount = 0;
  }
  
  lastWeight = weight;
  return stableCount >= requiredStableReadings;
}
