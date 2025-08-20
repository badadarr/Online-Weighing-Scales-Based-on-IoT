// Shared data types across modules
#pragma once
#include <Arduino.h>

// WeightData struct used across SensorReader and WebServer
struct WeightData {
  float raw;            // Raw HX711 data
  float filtered;       // Filtered weight
  float stable;         // Last stable value
  bool isStable;        // Stability flag
  bool hasMotion;       // Motion detected
  String quality;       // "stable", "motion", "error"
  unsigned long lastUpdate; // Last update timestamp
};
