#pragma once
#include <Arduino.h>

void initEEPROM();
bool isUIDStored(String uid);
void storeUID(String uid);
String getAllStoredUIDs(); // Returns JSON string of all stored UIDs
void saveCalibrationToEEPROM(float faktor);
float loadCalibrationFromEEPROM();
