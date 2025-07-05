#pragma once
#include <Arduino.h>

void initEEPROM();
bool isUIDStored(String uid);
void storeUID(String uid);
void saveCalibrationToEEPROM(float faktor);
float loadCalibrationFromEEPROM();
