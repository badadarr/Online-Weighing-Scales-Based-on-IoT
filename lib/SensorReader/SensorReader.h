#pragma once
#include <Arduino.h>
#include <HX711.h>

// Weight data structure
struct WeightData {
  float raw;
  float filtered;
  float stable;
  bool isStable;
  bool hasMotion;
  String quality;
  unsigned long lastUpdate;
};

// Advanced sensor functions
void setupSensor();
String readWeight();
void updateTareButton();
WeightData getAdvancedWeightData();
void performTare();
bool isWeightStable();
float getFilteredWeight();

// Calibration functions
void Kalibrasi(float knownWeight);
float getFaktorKalibrasi();
void setFaktorKalibrasi(float factor);
float loadCalibrationFromEEPROM();

// Advanced filtering and stability
void updateWeightBuffer(float newWeight);
float calculateMovingAverage();
float calculateMedianFilter();
bool detectMotion(float currentWeight);
String getWeightQuality();