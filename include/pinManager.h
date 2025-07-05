#pragma once
////// KONFIGURASI VARIABEL MAKRO //////
// ==== RFID (MFRC522) ====
#define RFID_RST_PIN     27     // Reset (pindah ke GPIO 27, misal)
#define RFID_SS_PIN      5      // SPI SS
#define RFID_SCK_PIN    18     // SPI SCK
#define RFID_MOSI_PIN   23     // SPI MOSI
#define RFID_MISO_PIN   19     // SPI MISO
#define RFID_IRQ_PIN     33     // IRQ (pindah ke GPIO 33, misal)

// ==== HX711 ====
#define HX711_DATA_PIN   25 // Pin Data HX711
#define HX711_CLOCK_PIN  26 // Pin Clock HX711
#define DEFAULT_FAKTOR_KALIBRASI  -94659.69f // Faktor Kalibrasi Default HX711
#define FAKTOR_KALIBRASI  -94659.69f //-94659.69f // Faktor Kalibrasi HX711
#define TARE_BUTTON_PIN  32  // Tombol Tare (GPIO 32)

// ==== RGB LED (Common Cathode) ====
#define LED_PIN_R      16 // Pin Merah
#define LED_PIN_G    4 // Pin Hijau
#define LED_PIN_B     2 // Pin Biru

// ==== BUZZER ====
#define BUZZER_PIN       17 // Pin Buzzer
#define BUZZER_BEEP_DURATION 100  // Durasi Beep Buzzer (ms)

// ==== LCD I2C ====
#define LCD_I2C_ADDRESS  0x27 // Alamat I2C LCD (sesuaikan dengan modulnya)
#define LCD_SDA_PIN      21     // Pin SDA untuk I2C (tetap)
#define LCD_SCL_PIN      22     // Pin SCL untuk I2C (tetap)

// ==== SERIAL CADANGAN ====
#define SERIAL_RX        16 //
#define SERIAL_TX        17 //
#define SERIAL_BAUDRATE  115200 // Baudrate Serial

// ==== LED Status Tambahan (opsional) ====
/************************ System Settings ***********************/
//#define WDT_TIMEOUT 10           // Watchdog timeout (detik)
#define EEPROM_SIZE 512
#define EEPROM_ADDR_KALIBRASI 0      // Alamat untuk faktor kalibrasi
#define EEPROM_ADDR_UID       32     // Alamat mulai untuk UID RFID
#define MAX_USERS 10             // Maksimal user RFID
#define WEIGHT_SAMPLES 10         // Jumlah sample pembacaan berat
#define WEIGHT_STABILIZE_DELAY 200 // Delay stabilitas berat (ms)
#define USERS_ADDR 16

/*********************** Color Codes (RGB) **********************/
// Warna RGB (untuk logika warna)
#define COLOR_OFF 0, 0, 0
#define COLOR_RED 255, 0, 0
#define COLOR_GREEN 0, 255, 0
#define COLOR_BLUE 0, 0, 255


// Makro kontrol pin LED
#define SET_LED_OFF() digitalWrite(LED_R_PIN, HIGH); digitalWrite(LED_G_PIN, HIGH); digitalWrite(LED_B_PIN, HIGH)
#define SET_LED_RED() digitalWrite(LED_R_PIN, LOW); digitalWrite(LED_G_PIN, HIGH); digitalWrite(LED_B_PIN, HIGH)
#define SET_LED_GREEN() digitalWrite(LED_R_PIN, HIGH); digitalWrite(LED_G_PIN, LOW); digitalWrite(LED_B_PIN, HIGH)
#define SET_LED_BLUE() digitalWrite(LED_R_PIN, HIGH); digitalWrite(LED_G_PIN, HIGH); digitalWrite(LED_B_PIN, LOW)
