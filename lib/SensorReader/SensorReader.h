#pragma once
#include <Arduino.h>
#include <HX711.h>

void setupSensor();
String readWeight();
void updateTareButton();

void Kalibrasi(float knownWeight);
float getFaktorKalibrasi();
void setFaktorKalibrasi(float factor);
float loadCalibrationFromEEPROM();