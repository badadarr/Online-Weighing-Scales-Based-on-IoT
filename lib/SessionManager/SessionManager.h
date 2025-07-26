#pragma once
#include <Arduino.h>
#include "config.h"

/**
 * @brief Class untuk mengelola sesi pengguna timbangan
 * @details Mengatur login/logout, timeout session, dan tracking aktivitas pengguna
 */
class SessionManager {
private:
    bool sessionActive;           // Status apakah ada sesi aktif
    String currentUserUID;        // UID pengguna yang sedang login
    unsigned long lastActivityTime;  // Timestamp aktivitas terakhir
    unsigned long sessionStartTime; // Timestamp mulai sesi

public:
    /**
     * @brief Constructor SessionManager
     * @details Inisialisasi semua variabel session ke state default
     */
    SessionManager();
    
    /**
     * @brief Login pengguna dengan UID RFID
     * @param uid String UID RFID pengguna
     * @return true jika login berhasil, false jika gagal
     * @details Memulai sesi baru dan mencatat waktu login
     */
    bool login(String uid);
    
    /**
     * @brief Logout pengguna dan mengakhiri sesi
     * @return true jika logout berhasil, false jika tidak ada sesi aktif
     * @details Membersihkan data sesi dan reset status
     */
    bool logout();
    
    /**
     * @brief Mengecek apakah ada sesi yang sedang aktif
     * @return true jika ada sesi aktif, false jika tidak
     * @details Untuk validasi akses sebelum menggunakan timbangan
     */
    bool isSessionActive();
    
    /**
     * @brief Mengecek apakah sesi sudah timeout
     * @return true jika sesi timeout dan perlu logout otomatis
     * @details Membandingkan waktu aktivitas terakhir dengan timeout limit
     */
    bool checkSessionTimeout();
    
    /**
     * @brief Update timestamp aktivitas terakhir
     * @details Dipanggil setiap ada interaksi untuk mencegah timeout
     */
    void updateActivity();
    
    /**
     * @brief Mendapatkan UID pengguna yang sedang login
     * @return String UID pengguna aktif, empty string jika tidak ada sesi
     * @details Untuk identifikasi pengguna di log dan display
     */
    String getCurrentUserUID();
    
    /**
     * @brief Mendapatkan durasi sesi saat ini dalam milidetik
     * @return unsigned long durasi sesi sejak login
     * @details Untuk monitoring dan statistik penggunaan
     */
    unsigned long getSessionDuration();
    
    /**
     * @brief Mencetak status sesi ke Serial untuk debugging
     * @details Menampilkan informasi lengkap tentang sesi aktif
     */
    void printSessionStatus();
};

/**
 * @brief Instance global SessionManager
 * @details Dapat diakses dari semua modul untuk manajemen sesi terpusat
 */
extern SessionManager sessionManager;