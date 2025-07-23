#pragma once
#include <Arduino.h>

void setupLCD();
void lcdShowStatus(String msg);
void lcdShowBerat(String berat);
void lcdShowTare(String berat); // Fungsi untuk menampilkan pesan tare selesai di LCD
void lcdShowRFID(String uid); // fungsi untuk menampilkan UID RFID di LCD
void lcdShowError(String error); // fungsi untuk menampilkan pesan error di LCD
void lcdShowWaiting(); // fungsi untuk menampilkan pesan menunggu RFID scan
void lcdShowReady(); // fungsi untuk menampilkan pesan siap
void lcdShowConnecting(); // fungsi untuk menampilkan status koneksi WiFi
void lcdShowFirebase(String status); // fungsi untuk menampilkan status Firebase
void lcdShowSyncTime(String status); // fungsi untuk menampilkan status sinkronisasi waktu
void lcdShowInit(String msg); // fungsi untuk menampilkan pesan inisialisasi
void lcdClear();
void lcdClearFirebaseLine(); // fungsi untuk membersihkan baris Firebase saja
void lcdShowQuality(String quality); // fungsi untuk menampilkan quality timbangan
void lcdShowLogoutInstructions(); // fungsi untuk menampilkan instruksi logout