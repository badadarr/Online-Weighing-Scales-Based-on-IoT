// File: NetworkManager.cpp

#include <WiFi.h>
#include "config.h"
#include "NetworkManager.h"
#include "lcd_display.h"
#include "indicator.h"

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Menghubungkan");
  lcdShowStatus("Menghubungkan WiFi...");
  LEDBuzz(50); // LED dan buzzer menyala sebagai tanda menghubungkan
  while (WiFi.status() != WL_CONNECTED) { // Tunggu hingga terhubung
    Serial.print(".");
    lcdShowStatus("Menghubungkan...");
    setColor(255, 0, 0); // Merah jika gagal
    buzz(100); // Bunyi buzzer sebagai tanda gagal
    delay(500);
  }
  Serial.println("\n[WiFi] Terhubung ke " + String(WIFI_SSID));
  lcdShowStatus("Koneksi Sukses!");
  setColor(0, 255, 0); // Hijau jika berhasil
  Serial.println("[WiFi] IP: " + WiFi.localIP().toString());
  lcdShowStatus("IP: " + WiFi.localIP().toString());
  delay(1000); // Tunda sejenak untuk memastikan pesan tampil
  setColor(0, 0, 0); // Matikan LED setelah koneksi
  
}
