#pragma once
#include <Arduino.h>
#include <HX711.h>

/**
 * @brief Struktur data untuk menyimpan informasi berat timbangan
 * @details Berisi data mentah, terfilter, status stabilitas, dan kualitas pembacaan
 */
struct WeightData {
  float raw;                    // Data mentah dari HX711 (tanpa filter)
  float filtered;               // Data setelah filtering (moving average + median)
  float stable;                 // Data stabil terakhir yang valid
  bool isStable;                // Status apakah pembacaan sudah stabil
  bool hasMotion;               // Deteksi gerakan/perubahan berat
  String quality;               // Kualitas pembacaan: "stable", "motion", "error"
  unsigned long lastUpdate;     // Timestamp update terakhir
};

/**
 * @brief Inisialisasi sensor HX711 dan konfigurasi pin
 * @details Setup komunikasi dengan load cell, set gain, dan kalibrasi awal
 */
void setupSensor();

/**
 * @brief Membaca berat dalam format string untuk kompatibilitas legacy
 * @return String berat dalam kg dengan 3 desimal
 * @details Fungsi wrapper untuk backward compatibility
 */
String readWeight();

/**
 * @brief Update status tombol tare fisik (jika ada)
 * @details Mengecek input tombol tare dan melakukan tare otomatis
 */
void updateTareButton();

/**
 * @brief Mengambil data berat lengkap dengan analisis kualitas
 * @return WeightData struktur berisi semua informasi berat
 * @details Fungsi utama untuk mendapatkan data berat dengan filtering dan validasi
 */
WeightData getAdvancedWeightData();

/**
 * @brief Melakukan tare (reset zero point) pada timbangan
 * @details Mengatur titik nol baru berdasarkan pembacaan saat ini
 */
void performTare();

/**
 * @brief Mengecek apakah pembacaan berat sudah stabil
 * @return true jika stabil, false jika masih berfluktuasi
 * @details Menggunakan algoritma deteksi stabilitas berdasarkan variance
 */
bool isWeightStable();

/**
 * @brief Mendapatkan berat yang sudah difilter
 * @return float berat dalam kg setelah filtering
 * @details Menggunakan kombinasi moving average dan median filter
 */
float getFilteredWeight();

/**
 * @brief Melakukan kalibrasi dengan berat standar yang diketahui
 * @param knownWeight Berat standar dalam kg untuk referensi kalibrasi
 * @details Menghitung dan menyimpan faktor kalibrasi baru ke EEPROM
 */
void Kalibrasi(float knownWeight);

/**
 * @brief Mendapatkan faktor kalibrasi saat ini
 * @return float nilai faktor kalibrasi yang sedang digunakan
 * @details Faktor ini mengkonversi raw data HX711 ke satuan kg
 */
float getFaktorKalibrasi();

/**
 * @brief Mengatur faktor kalibrasi baru
 * @param factor Nilai faktor kalibrasi baru
 * @details Mengubah faktor kalibrasi dan menyimpannya ke EEPROM
 */
void setFaktorKalibrasi(float factor);

/**
 * @brief Memuat faktor kalibrasi dari EEPROM saat startup
 * @return float faktor kalibrasi yang tersimpan
 * @details Dipanggil saat inisialisasi untuk restore pengaturan kalibrasi
 */
float loadCalibrationFromEEPROM();

/**
 * @brief Update buffer data berat untuk filtering
 * @param newWeight Data berat baru yang akan ditambahkan ke buffer
 * @details Mengelola circular buffer untuk algoritma filtering
 */
void updateWeightBuffer(float newWeight);

/**
 * @brief Menghitung rata-rata bergerak dari buffer data
 * @return float hasil moving average
 * @details Mengurangi noise dengan averaging beberapa sample terakhir
 */
float calculateMovingAverage();

/**
 * @brief Menghitung median filter untuk menghilangkan outlier
 * @return float hasil median filtering
 * @details Efektif menghilangkan spike dan noise impulsif
 */
float calculateMedianFilter();

/**
 * @brief Mendeteksi adanya gerakan atau perubahan berat
 * @param currentWeight Berat saat ini untuk dibandingkan
 * @return true jika terdeteksi motion, false jika statis
 * @details Menggunakan threshold untuk mendeteksi perubahan signifikan
 */
bool detectMotion(float currentWeight);

/**
 * @brief Menentukan kualitas pembacaan berat saat ini
 * @return String status kualitas: "stable", "motion", "stabilizing", "error"
 * @details Memberikan feedback kualitas untuk user interface
 */
String getWeightQuality();