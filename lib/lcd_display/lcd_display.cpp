#include "lcd_display.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setupLCD() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Timbangan Online");
  lcd.setCursor(0, 1);
  lcd.print("  by VeroScale  ");
  delay(2000);
  lcd.clear();
}
void lcdClear(){
  lcd.clear();
  lcd.setCursor(0, 0);
}
void lcdShowStatus(String msg) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Status:");
  lcd.setCursor(0, 1);
  lcd.print(msg.substring(0, 16)); // Tampilkan pesan status
  //lcd.clear();
}

void lcdShowFirebase(String dt) {
  //lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("RTDB:");
  lcd.setCursor(6, 1);
  lcd.print(dt.substring(0, 16));
}

void lcdShowBerat(String berat) {
  //lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Berat: ");
  lcd.setCursor(7, 0);
  lcd.print(berat);
  lcd.print(" kg   ");
}
void lcdShowTare(String berat) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tare Selesai...");  
}
void lcdShowRFID(String uid) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID UID: ");
  lcd.print(uid.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print("Scan Selesai");
}
void lcdShowError(String error) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Error:");
  lcd.setCursor(0, 1);
  lcd.print(error.substring(0, 16));
}
void lcdShowWaiting() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Menunggu RFID...");
  lcd.setCursor(0, 1);
  lcd.print("Silakan Scan");
}
void lcdShowReady() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Siap Scan RFID");
  lcd.setCursor(0, 1);
  lcd.print("Tunggu RFID...");
}
void lcdShowConnecting() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Menghubungkan...");
  lcd.setCursor(0, 1);
  lcd.print("Silakan Tunggu");
}

void lcdShowSyncTime(String status) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sync Waktu:");
  lcd.setCursor(0, 1);
  lcd.print(status.substring(0, 16));
}

