#pragma once
#include <Arduino.h>

/**
 * @brief Inisialisasi modul RFID MFRC522
 * @details Setup komunikasi SPI dengan reader RFID, konfigurasi pin, dan test koneksi
 */
void setupRFID();

/**
 * @brief Validasi format UID RFID yang dibaca
 * @param uid Reference ke string UID yang akan divalidasi dan dinormalisasi
 * @return true jika UID valid (8 karakter hex), false jika tidak valid
 * @details Mengecek format hex dan panjang UID, serta normalisasi ke uppercase
 */
bool isRFIDValid(String &uid);

/**
 * @brief Mengumpulkan data pengguna RFID dari Firebase
 * @return true jika berhasil mengumpulkan data, false jika gagal
 * @details Download daftar pengguna authorized dari Firebase ke cache lokal
 */
bool collectRFIDUsersData();

/**
 * @brief Sinkronisasi data pengguna RFID dari Firebase
 * @return true jika sinkronisasi berhasil, false jika gagal
 * @details Update cache lokal dengan data terbaru dari Firebase database
 */
bool syncRFIDUsersFromFirebase();

/**
 * @brief Mengecek apakah data pengguna RFID sudah di-cache
 * @return true jika data sudah tersedia di cache, false jika belum
 * @details Digunakan untuk menentukan apakah perlu download data dari Firebase
 */
bool isRFIDUsersDataCached();

/**
 * @brief Membersihkan cache data pengguna RFID
 * @details Menghapus semua data pengguna dari memory untuk refresh ulang
 */
void clearRFIDUsersCache();

/**
 * @brief Mendapatkan jumlah pengguna yang ter-cache
 * @return int jumlah pengguna RFID yang tersimpan di cache
 * @details Untuk monitoring dan display informasi sistem
 */
int getCachedUsersCount();

/**
 * @brief Mengecek apakah UID RFID memiliki akses authorized
 * @param uid String UID RFID yang akan dicek (8 karakter hex)
 * @return true jika UID authorized, false jika tidak
 * @details Mengecek di cache lokal dan Firebase untuk validasi akses
 */
bool isUIDAuthorized(String uid);

/**
 * @brief Meminta registrasi UID RFID baru ke sistem
 * @param uid String UID yang akan didaftarkan
 * @details Mengirim request ke Firebase untuk menambahkan UID baru
 */
void requestRFIDRegistration(String uid);

/**
 * @brief Memberikan akses timbangan untuk UID tertentu
 * @param uid String UID yang akan diberikan akses
 * @return true jika akses berhasil diberikan, false jika gagal
 * @details Memulai sesi pengguna dan mengaktifkan akses timbangan
 */
bool grantWeighingAccess(String uid);

/**
 * @brief Reset akses timbangan (logout)
 * @details Menghentikan sesi aktif dan menonaktifkan akses timbangan
 */
void resetAccess();

/**
 * @brief Mengecek apakah akses timbangan sedang aktif
 * @return true jika ada sesi aktif, false jika tidak ada akses
 * @details Untuk validasi sebelum mengizinkan penggunaan timbangan
 */
bool isWeighingAccessGranted();

/**
 * @brief Mendapatkan nama pengguna yang sedang mengakses
 * @return String nama pengguna atau UID jika nama tidak tersedia
 * @details Untuk display di LCD dan web interface
 */
String getCurrentAuthorizedUser();

/**
 * @brief Memperpanjang waktu akses pengguna aktif
 * @details Reset timer timeout untuk mencegah auto-logout
 */
void extendAccess();

/**
 * @brief Memproses tag RFID yang baru dibaca
 * @param uid String UID dari tag RFID
 * @return true jika berhasil diproses, false jika gagal
 * @details Fungsi utama untuk handling semua logic RFID
 */
bool processRFIDTag(String uid);

/**
 * @brief Handler utama untuk akses RFID
 * @details Loop function untuk membaca tag RFID dan memproses akses
 */
void handleRFIDAccess();

// ===== LEGACY FUNCTIONS FOR BACKWARD COMPATIBILITY =====

/**
 * @brief [LEGACY] Mengecek apakah UID terdaftar
 * @param uid String UID RFID
 * @return true jika terdaftar, false jika tidak
 * @deprecated Gunakan isUIDAuthorized() sebagai gantinya
 */
bool isUIDRegistered(String uid);

/**
 * @brief [LEGACY] Handle login RFID
 * @param uid String UID RFID
 * @return true jika login berhasil, false jika gagal
 * @deprecated Gunakan grantWeighingAccess() sebagai gantinya
 */
bool handleRFIDLogin(String uid);

/**
 * @brief [LEGACY] Handle logout RFID
 * @param uid String UID RFID
 * @return true jika logout berhasil, false jika gagal
 * @deprecated Gunakan resetAccess() sebagai gantinya
 */
bool handleRFIDLogout(String uid);