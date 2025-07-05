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
// File: src/main.cpp
unsigned long lastUpdate = 0;
String lastUID = "";
bool sendingActive = false; // Status pengiriman data

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
  
  lcdShowStatus("Siap digunakan...");  
  ulangiBuzzer();
  lcdClear();

}
void loop() {

  String berat = readWeight(); // Baca berat dari sensor load cell
  Serial.println("[HX711] Berat: " + berat + " kg");
  lcdShowBerat(berat); // Tampilkan berat di LCD
  if (berat.toFloat() > 0) { // Jika berat valid
    sendBeratKeFirebase(berat); // Kirim berat ke Firebase
    //delay(10);
  }else{
    setColor(0, 0, 0); // Merah jika berat tidak valid
    buzz(100); // Bunyi buzzer sebagai tanda berat tidak valid
    lcdShowFirebase("Stand by...");
    return; // Keluar dari loop jika berat tidak valid
  }
  updateTareButton(); // Perbarui tombol tare
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
        Serial.println("[RFID] UID sudah aktif: " + uid);
        setColor(255, 255, 0);
        lastUID = "";
        delay(500);
        return;
      } else {
        sendingActive = true; // Aktifkan pengiriman
        lastUID = uid;
        Serial.println("[RFID] UID Terdeteksi: " + uid);
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

  delay(200);
}
