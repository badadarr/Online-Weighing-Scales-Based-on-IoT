// file: include/FirebaseClient.h
#pragma once
#include <Arduino.h>
#include "config.h" // gunakan tanda kutip, bukan <>
//#include <FirebaseESP32.h>
#include <Firebase_ESP_Client.h> // Include Firebase ESP Client library
extern FirebaseData fbdo;

void setupFirebase();
void sendBeratKeFirebase(const String& berat);
void cleanupOldData(); // Kirim berat ke Firebase

