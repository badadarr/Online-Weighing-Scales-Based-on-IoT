/**
 * @file config.h
 * @brief File konfigurasi utama sistem timbangan IoT
 * @details Berisi semua konstanta, parameter, dan konfigurasi yang digunakan
 *          di seluruh sistem termasuk WiFi, Firebase, hardware, dan performance
 */

#pragma once

// ==================== NETWORK CONFIGURATION ====================
/**
 * @brief Konfigurasi koneksi WiFi
 * @details SSID dan password untuk koneksi ke jaringan WiFi lokal
 */
#define WIFI_SSID "redmi_9"        // Nama jaringan WiFi
#define WIFI_PASSWORD "astagfirullah"    // Password jaringan WiFi

// ==================== FIREBASE CONFIGURATION ====================
/**
 * @brief Konfigurasi Firebase Realtime Database
 * @details Kredensial dan URL untuk koneksi ke Firebase cloud database
 */
#define API_KEY "AIzaSyCpT8kEDaXLxqTavO4Als-w7Wn4BDcyRMM"  // Firebase API Key
#define DATABASE_URL "https://iot-scales-enhancement-default-rtdb.asia-southeast1.firebasedatabase.app"  // Firebase Database URL
#define USER_EMAIL "esp32@timbangan-iot.com"     // Email untuk autentikasi Firebase
#define USER_PASSWORD "esp32secure123"           // Password untuk autentikasi Firebase

// ==================== DEVICE IDENTIFICATION ====================
/**
 * @brief Identifikasi unik perangkat
 * @details ID unik untuk identifikasi perangkat di Firebase dan logging
 */
#define DEVICE_ID "esp32_timbangan_001"  // ID unik perangkat timbangan

// ==================== TIMING CONFIGURATION ====================
/**
 * @brief Interval update data ke Firebase dalam milidetik
 * @details Mengatur seberapa sering data dikirim ke cloud database
 */
#define FIREBASE_UPDATE_INTERVAL 5000     // Update setiap 5 detik

/**
 * @brief Konfigurasi Network Time Protocol (NTP)
 * @details Pengaturan sinkronisasi waktu dengan server NTP
 */
#define NTP_SERVER "pool.ntp.org"         // Server NTP untuk sinkronisasi waktu
#define GMT_OFFSET_SEC 25200              // Offset GMT+7 untuk WIB (7 * 3600)
#define DAYLIGHT_OFFSET_SEC 0             // Tidak ada daylight saving time

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
#define EEPROM_ADDR_WIFI_SSID 8
#define EEPROM_ADDR_WIFI_PASSWORD 72  // SSID max 64 bytes
#define EEPROM_ADDR_WIFI_CONFIGURED 136  // 1 byte flag

// Performance Configuration (OPTIMIZED FOR STABILITY)
#define LOOP_DELAY_MS 100           // Increased loop delay to reduce CPU load
#define WEIGHT_READ_INTERVAL_MS 200 // Reduced weight reading frequency
#define WEB_UPDATE_INTERVAL_MS 1000 // Reduced web update frequency
#define LCD_UPDATE_INTERVAL_MS 500  // Reduced LCD update frequency

// Task Management Configuration
#define WEB_SERVER_CHECK_INTERVAL_MS 100    // Web server check interval
#define FIREBASE_SYNC_INTERVAL_MS 15000     // Firebase sync interval (15 seconds)
#define WATCHDOG_FEED_INTERVAL_MS 1000      // Feed watchdog every 1 second
#define TASK_YIELD_DELAY_MS 10              // Yield delay between tasks

// Memory Management
#define ENABLE_MEMORY_OPTIMIZATION 1        // Enable memory optimizations
#define REDUCE_JSON_BUFFER_SIZE 1          // Use smaller JSON buffers
#define LIMIT_CONCURRENT_OPERATIONS 1       // Limit concurrent operations

// Logging Configuration
#define ENABLE_DETAILED_LOGGING 1
#define ENABLE_BASE_CORRECTION_LOG 1
#define ENABLE_MOTION_LED 1
#define LOG_CHANGE_THRESHOLD 0.1

// Firebase Configuration
#define FIREBASE_MIN_CHANGE 0.05
#define FIREBASE_MAX_INTERVAL_MS 30000

// Buzzer Sound Patterns
#define BUZZ_STANDBY 0          // No sound for standby
#define BUZZ_STABILIZING 50     // Short beep for stabilizing
#define BUZZ_MOTION 100         // Medium beep for motion
#define BUZZ_WAITING 200        // Long beep for waiting stable
#define BUZZ_SUCCESS 300        // Success confirmation
#define BUZZ_ERROR 500          // Error alert

// Session Configuration
#define SESSION_TIMEOUT_MS 300000  // Auto logout after 5 minutes of inactivity
#define SESSION_ACTIVE_LED COLOR_GREEN  // LED color when session is active
#define SESSION_INACTIVE_LED COLOR_RED  // LED color when session is inactive
#define SESSION_LOGIN_SOUND BUZZ_SUCCESS  // Sound when logging in
#define SESSION_LOGOUT_SOUND BUZZ_ERROR  // Sound when logging out

// ==================== DEMO/BYPASS CONFIGURATION ====================
/**
 * @brief RFID bypass mode for demo/emergency
 * @details Set BYPASS_RFID to 1 to skip RFID and allow weighing/sending without user session.
 *          Disable by setting to 0 once hardware is ready.
 */
#ifndef BYPASS_RFID
#define BYPASS_RFID 1
#endif

// Displayed name/UID when bypass is active
#ifndef BYPASS_USER_NAME
#define BYPASS_USER_NAME "Guest"
#endif

#ifndef BYPASS_USER_UID
#define BYPASS_USER_UID "BYPASS"
#endif