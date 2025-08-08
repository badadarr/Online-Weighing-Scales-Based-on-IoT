/**
 * @file test_pins.cpp
 * @brief Test file untuk verifikasi pin assignments dari pinManager.h
 * @details File ini untuk memastikan semua pin sudah terdefinisi dengan benar
 */

#include "pinManager.h"
#include <Arduino.h>

void testPinDefinitions() {
  Serial.println("=== PIN ASSIGNMENT TEST ===");
  
  // Test RFID pins
  Serial.println("RFID Pins:");
  Serial.println("  RST: " + String(RFID_RST_PIN));
  Serial.println("  SS:  " + String(RFID_SS_PIN));
  Serial.println("  SCK: " + String(RFID_SCK_PIN));
  Serial.println("  MOSI:" + String(RFID_MOSI_PIN));
  Serial.println("  MISO:" + String(RFID_MISO_PIN));
  
  // Test HX711 pins
  Serial.println("HX711 Pins:");
  Serial.println("  DATA: " + String(HX711_DATA_PIN));
  Serial.println("  CLOCK:" + String(HX711_CLOCK_PIN));
  
  // Test Button pins
  Serial.println("Button Pins:");
  Serial.println("  TARE: " + String(TARE_BUTTON_PIN));
  Serial.println("  UP:   " + String(UP_BUTTON_PIN));
  Serial.println("  DOWN: " + String(DOWN_BUTTON_PIN));
  Serial.println("  ENTER:" + String(ENTER_BUTTON_PIN));
  
  // Test LED pins
  Serial.println("LED Pins:");
  Serial.println("  RED:  " + String(LED_PIN_R));
  Serial.println("  GREEN:" + String(LED_PIN_G));
  Serial.println("  BLUE: " + String(LED_PIN_B));
  
  // Test other pins
  Serial.println("Other Pins:");
  Serial.println("  BUZZER:" + String(BUZZER_PIN));
  Serial.println("  LCD SDA:" + String(LCD_SDA_PIN));
  Serial.println("  LCD SCL:" + String(LCD_SCL_PIN));
  
  Serial.println("=== TEST COMPLETE ===");
}

void checkPinConflicts() {
  Serial.println("=== PIN CONFLICT CHECK ===");
  
  int pins[] = {
    RFID_RST_PIN, RFID_SS_PIN, RFID_SCK_PIN, RFID_MOSI_PIN, RFID_MISO_PIN,
    HX711_DATA_PIN, HX711_CLOCK_PIN,
    TARE_BUTTON_PIN, UP_BUTTON_PIN, DOWN_BUTTON_PIN, ENTER_BUTTON_PIN,
    LED_PIN_R, LED_PIN_G, LED_PIN_B,
    BUZZER_PIN, LCD_SDA_PIN, LCD_SCL_PIN
  };
  
  int pinCount = sizeof(pins) / sizeof(pins[0]);
  bool conflict = false;
  
  for (int i = 0; i < pinCount; i++) {
    for (int j = i + 1; j < pinCount; j++) {
      if (pins[i] == pins[j]) {
        Serial.println("CONFLICT: Pin " + String(pins[i]) + " used multiple times!");
        conflict = true;
      }
    }
  }
  
  if (!conflict) {
    Serial.println("No pin conflicts detected!");
  }
  
  Serial.println("=== CONFLICT CHECK COMPLETE ===");
}