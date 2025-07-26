#pragma once
#include <Arduino.h>

/**
 * @brief Inisialisasi EEPROM untuk penyimpanan data lokal
 * @details Mengalokasikan memori EEPROM dan mempersiapkan area penyimpanan
 *          untuk UID RFID dan data kalibrasi timbangan
 */
void initEEPROM();

/**
 * @brief Mengecek apakah UID RFID sudah tersimpan di EEPROM
 * @param uid String UID RFID yang akan dicek (format hex 8 karakter)
 * @return true jika UID sudah tersimpan, false jika belum
 * @details Fungsi ini digunakan untuk validasi akses pengguna secara offline
 */
bool isUIDStored(String uid);

/**
 * @brief Menyimpan UID RFID baru ke EEPROM
 * @param uid String UID RFID yang akan disimpan (format hex 8 karakter)
 * @details Menambahkan UID ke daftar pengguna yang diizinkan menggunakan timbangan
 *          Data disimpan secara permanen di EEPROM ESP32
 */
void storeUID(String uid);

/**
 * @brief Mengambil semua UID yang tersimpan dalam format JSON
 * @return String JSON berisi array semua UID yang tersimpan
 * @details Format output: {"uids":["12345678","ABCDEF01",...]}
 *          Digunakan untuk sinkronisasi dengan web interface
 */
String getAllStoredUIDs();

/**
 * @brief Menyimpan faktor kalibrasi timbangan ke EEPROM
 * @param faktor Nilai faktor kalibrasi (float) untuk konversi raw data ke kg
 * @details Faktor kalibrasi menentukan akurasi pembacaan berat timbangan
 *          Disimpan permanen untuk mempertahankan akurasi setelah restart
 */
void saveCalibrationToEEPROM(float faktor);

/**
 * @brief Memuat faktor kalibrasi dari EEPROM
 * @return float Nilai faktor kalibrasi yang tersimpan
 * @details Dipanggil saat startup untuk mengembalikan pengaturan kalibrasi
 *          Return default value jika belum pernah dikalibrasi
 */
float loadCalibrationFromEEPROM();
