#pragma once

// WiFi
#define WIFI_SSID "HUAWEI-qKN3"
#define WIFI_PASSWORD "calyaf1886jj"

// Firebase Project
#define API_KEY "AIzaSyDwdYrP2FEYV2hAS1QrYEcjXDJqcvUI4WQ"
#define DATABASE_URL "https://timbangan-online-3cd46-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "admin@contoh.com"
#define USER_PASSWORD "admin123"

// Perangkat
#define DEVICE_ID "esp32_timbangan_001"

// Waktu update data ke Firebase (ms)
#define FIREBASE_UPDATE_INTERVAL 5000

// Konfigurasi waktu (NTP)
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 25200 // GMT+7
#define DAYLIGHT_OFFSET_SEC 0

// Web Server Configuration
#define WEB_SERVER_PORT 80
#define WEB_USERNAME "admin"
#define WEB_PASSWORD "timbangan123"

// Base/Tatakan Configuration
#define DEFAULT_BASE_MODE false
#define DEFAULT_BASE_WEIGHT 0.0f
#define BASE_CALIBRATION_SAMPLES 10

// Web API Endpoints
#define API_STATUS "/api/status"
#define API_CONFIG "/api/config"
#define API_CALIBRATE "/api/calibrate"
#define API_RESET "/api/reset"

// System Configuration
#define WEIGHT_PRECISION 3
#define WEIGHT_UNIT "kg"
#define DEVICE_NAME "Timbangan IoT"
#define FIRMWARE_VERSION "1.2.0"

// EEPROM Configuration
#define EEPROM_SIZE 512
#define EEPROM_ADDR_BASE_MODE 0
#define EEPROM_ADDR_BASE_WEIGHT 4

// Performance Configuration (BALANCED MODE)
#define LOOP_DELAY_MS 50
#define WEIGHT_READ_INTERVAL_MS 100
#define WEB_UPDATE_INTERVAL_MS 500
#define LCD_UPDATE_INTERVAL_MS 200

// Logging Configuration
#define ENABLE_DETAILED_LOGGING 1
#define ENABLE_BASE_CORRECTION_LOG 1
#define ENABLE_MOTION_LED 1
#define LOG_CHANGE_THRESHOLD 0.1

// Firebase Configuration
#define FIREBASE_MIN_CHANGE 0.05
#define FIREBASE_MAX_INTERVAL_MS 30000