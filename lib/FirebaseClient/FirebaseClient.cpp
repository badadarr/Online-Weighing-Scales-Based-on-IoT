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

  // Tampilkan waktu sistem (debug)
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("[NTP] Gagal mendapatkan waktu!");
  } else {
    Serial.printf("[NTP] Waktu sekarang: %s", asctime(&timeinfo));
  }

  // Konfigurasi Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  //config.ssl_buffer_size = 4096; // buffer besar untuk stabilitas SSL
  //config.token_status_callback = tokenStatusCallback; // opsional

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Mulai koneksi Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  delay(500); // beri waktu untuk inisialisasi SSL

  fbdo.setResponseSize(1024); // Ukuran buffer untuk response JSON
  fbdo.setBSSLBufferSize(2048, 1024);   // RX dan TX buffer SSL

  // Cek memori heap
  Serial.printf("[Heap] Free heap: %d bytes\n", ESP.getFreeHeap());

  // Tunggu Firebase siap
  Serial.print("[Firebase] Autentikasi...");
  lcdShowFirebase("Auth Firebase...");

  unsigned long start = millis();
  while (!Firebase.ready()) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 10000) {
      Serial.println("\n[Firebase] Gagal login, restart ESP...");
      Serial.println("[Firebase] Alasan: " + fbdo.errorReason());
      lcdShowFirebase("Firebase Gagal!");
      delay(3000);
      ESP.restart();
    }
  }

  Serial.println("\n[Firebase] Siap kirim data.");
  lcdShowFirebase("Firebase Siap..");
}

void sendBeratKeFirebase(const String& berat) { // Kirim data berat ke Firebase
  if (!Firebase.ready()) 
  { // Jika Firebase belum siap
    firebaseFailCount++;
    Serial.println("[Firebase] Token belum siap.");
    lcdShowFirebase("Token blm siap..");

    if (firebaseFailCount >= MAX_FAILS_BEFORE_RESTART) 
    { // Jika gagal terlalu banyak
      Serial.println("[Firebase] Terlalu banyak gagal, restart...");
      lcdShowFirebase("Restarting...");
      LEDBuzz(100); // LED dan buzzer menyala sebagai tanda restart
      buzz(100); // Bunyi buzzer sebagai tanda restart
      delay(2000);
      ESP.restart(); // Restart ESP jika gagal terlalu banyak
    }
    return; // Jika Firebase belum siap, keluar dari fungsi
  }

  firebaseFailCount = 0; // Reset fail count jika berhasil
  String path = "/devices/" + String(DEVICE_ID) + "/berat_terakhir"; // Buat path untuk data berat

  fbdo.clear(); // reset koneksi FirebaseData
  if (Firebase.RTDB.setString(&fbdo, path, berat)) {
    Serial.println("[Firebase] Berat terkirim: " + berat);
    lcdShowFirebase("Kirim.. ");
    setColor(0, 255, 0); // Hijau jika berhasil
  } else {
    Serial.println("[Firebase] Gagal kirim berat: " + fbdo.errorReason());
    lcdShowFirebase("Kirim gagal..!");
    setColor(255, 0, 0); // Merah jika gagal
    buzz(200); 
    ESP.restart(); // Restart ESP jika gagal terlalu banyak
  }

  String logPath = "/devices/" + String(DEVICE_ID) + "/log/" + String(millis()); // Buat path log dengan timestamp
  fbdo.clear(); // reset koneksi FirebaseData
  if (Firebase.RTDB.setString(&fbdo, logPath, berat)) {
    Serial.println("[Firebase] Log berat terkirim.");
  } else {
    Serial.println("[Firebase] Gagal kirim log: " + fbdo.errorReason());
  }
}