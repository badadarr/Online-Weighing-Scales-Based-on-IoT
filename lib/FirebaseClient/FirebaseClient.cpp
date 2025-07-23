//File: src/FirebaseClient.cpp

#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif

// File: src/FirebaseClient.cpp
#include <Arduino.h> // Include Arduino core library
#include <Firebase_ESP_Client.h> // Include Firebase ESP Client library
#include "FirebaseClient.h" // file: include/FirebaseClient.h
#include "config.h" // file: include/config.h
#include "lcd_display.h" // file: include/lcd_display.h
#include "Indicator.h" // file: include/Indicator.h
#include "pinManager.h" // file: include/pinManager.h

FirebaseData fbdo; // Data object untuk Firebase
FirebaseAuth auth; // Autentikasi firebase
FirebaseConfig config; // konfigurasi Firebase

int firebaseFailCount = 0;
const int MAX_FAILS_BEFORE_RESTART = 3;

void setupFirebase() {
  Serial.println("[Firebase] Inisialisasi...");

  // Sinkronisasi waktu NTP (penting untuk SSL)
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  Serial.println("[NTP] Sinkronisasi waktu...");

  // Tunggu sinkronisasi waktu selesai (penting untuk SSL)
  int ntpRetries = 0;
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo) && ntpRetries < 10) {
    Serial.println("[NTP] Menunggu sinkronisasi waktu...");
    lcdShowStatus("Sync NTP...");
    delay(1000);
    ntpRetries++;
  }
  
  if (ntpRetries < 10) {
    Serial.printf("[NTP] Waktu sekarang: %s", asctime(&timeinfo));
  } else {
    Serial.println("[NTP] Gagal sinkronisasi waktu, melanjutkan...");
  }

  // Konfigurasi Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.timeout.serverResponse = 10000; // Timeout 10 detik
  config.timeout.wifiReconnect = 10000;  // Timeout 10 detik

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Mulai koneksi Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  delay(1000); // beri waktu lebih untuk inisialisasi SSL

  fbdo.setResponseSize(1024); // Ukuran buffer untuk response JSON
  fbdo.setBSSLBufferSize(2048, 1024);   // RX dan TX buffer SSL

  // Cek memori heap
  Serial.printf("[Heap] Free heap: %d bytes\n", ESP.getFreeHeap());

  // Tunggu Firebase siap dengan retry yang lebih lama
  Serial.print("[Firebase] Autentikasi...");
  lcdShowQuality("Auth Firebase");

  int authRetries = 0;
  const int MAX_AUTH_RETRIES = 5;
  
  while (!Firebase.ready() && authRetries < MAX_AUTH_RETRIES) {
    Serial.print(".");
    lcdShowStatus("Auth Retry " + String(authRetries + 1));
    delay(2000); // Tunggu lebih lama antara percobaan
    authRetries++;
  }

  if (Firebase.ready()) {
    Serial.println("\n[Firebase] Autentikasi berhasil!");
    lcdShowQuality("Firebase OK");
  } else {
    Serial.println("\n[Firebase] Autentikasi gagal setelah " + String(MAX_AUTH_RETRIES) + " percobaan");
    Serial.println("[Firebase] Alasan: " + fbdo.errorReason());
    lcdShowQuality("FB Offline");
    // Tidak restart, biarkan sistem berjalan dalam mode offline
  }

  Serial.println("[Firebase] Siap kirim data.");
}

void sendBeratKeFirebase(const String& berat) {
  if (!Firebase.ready()) {
    firebaseFailCount++;
    Serial.println("[Firebase] Token belum siap. Percobaan: " + String(firebaseFailCount));
    lcdShowQuality("Offline");
    
    // Jangan restart, hanya log pesan error
    if (firebaseFailCount >= MAX_FAILS_BEFORE_RESTART) {
      Serial.println("[Firebase] Firebase offline, melanjutkan dalam mode lokal");
      firebaseFailCount = 0; // Reset counter untuk mencegah spam log
    }
    return;
  }

  firebaseFailCount = 0;
  
  // Struktur data yang lebih efisien
  FirebaseJson json;
  unsigned long timestamp = millis();
  String sessionId = String(timestamp);
  
  // Data lengkap untuk session
  json.set("weight", berat);
  json.set("timestamp", timestamp);
  json.set("device_id", DEVICE_ID);
  json.set("quality", "stable");
  
  // Update current status (real-time) - with frequency limit
  static unsigned long lastFirebaseUpdate = 0;
  String currentPath = "/devices/" + String(DEVICE_ID) + "/current";
  fbdo.clear();
  if (Firebase.RTDB.setJSON(&fbdo, currentPath, &json)) {
    // Only log successful updates every 10 seconds to reduce spam
    if (millis() - lastFirebaseUpdate > 10000) {
      Serial.println("[Firebase] Current data updated: " + berat);
      lastFirebaseUpdate = millis();
    }
    lcdShowQuality("Sent OK");
    setColor(0, 255, 0);
  } else {
    Serial.println("[Firebase] Failed: " + fbdo.errorReason());
    lcdShowQuality("Error");
    setColor(255, 0, 0);
    return;
  }
  
  // Simpan ke history dengan auto-cleanup (hanya 50 data terakhir)
  String historyPath = "/devices/" + String(DEVICE_ID) + "/history/" + sessionId;
  fbdo.clear();
  Firebase.RTDB.setJSON(&fbdo, historyPath, &json);
  
  // Auto-cleanup: hapus data lama (opsional)
  static unsigned long lastCleanup = 0;
  if (millis() - lastCleanup > 300000) { // Cleanup setiap 5 menit
    cleanupOldData();
    lastCleanup = millis();
  }
}

void cleanupOldData() {
  String historyPath = "/devices/" + String(DEVICE_ID) + "/history";
  fbdo.clear();
  if (Firebase.RTDB.getJSON(&fbdo, historyPath)) {
    FirebaseJson json = fbdo.jsonObject();
    size_t count = json.iteratorBegin();
    if (count > 50) {
      String oldestKey;
      unsigned long oldestTime = ULONG_MAX;
      
      for (size_t i = 0; i < count; i++) {
        int type;
        String key, value;
        json.iteratorGet(i, type, key, value);
        unsigned long time = key.toInt();
        if (time < oldestTime) {
          oldestTime = time;
          oldestKey = key;
        }
      }
      
      if (oldestKey.length() > 0) {
        Firebase.RTDB.deleteNode(&fbdo, historyPath + "/" + oldestKey);
        Serial.println("[Firebase] Cleaned: " + oldestKey);
      }
    }
    json.iteratorEnd();
  }
}