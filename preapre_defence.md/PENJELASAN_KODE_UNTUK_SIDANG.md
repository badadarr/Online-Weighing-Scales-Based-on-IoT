# PENJELASAN KODE UNTUK PERSIAPAN SIDANG
## Sistem Timbangan IoT dengan Kontrol Akses RFID

### 1. ARSITEKTUR KODE

#### A. Struktur Modular
```
lib/
├── SensorReader/     → Pembacaan sensor HX711 + filtering
├── RFIDReader/       → Kontrol akses RFID MFRC522
├── SessionManager/   → Manajemen sesi pengguna
├── WebServer/        → REST API + web interface
├── FirebaseClient/   → Koneksi cloud database
├── LocalStorage/     → Penyimpanan EEPROM
├── Indicator/        → LED + buzzer feedback
└── lcd_display/      → Display LCD I2C
```

**Keuntungan Modular:**
- Setiap modul independen dan reusable
- Mudah debugging dan maintenance
- Scalable untuk pengembangan future

#### B. Flow Data Utama
```
RFID Scan → Session Check → Sensor Read → Data Filter → 
Stability Check → Firebase Send → LCD/Web Update
```

### 2. FUNGSI-FUNGSI KUNCI YANG HARUS DIPAHAMI

#### A. **main.cpp - Program Utama**
```cpp
void setup() {
    // Inisialisasi semua hardware dan software
    // WiFi → Firebase → Sensor → RFID → Web Server
}

void loop() {
    // 1. Watchdog feeding (mencegah system reset)
    // 2. RFID access control
    // 3. Weight reading + filtering
    // 4. Stability detection
    // 5. Firebase data sending
    // 6. Web server handling
}
```

**Poin Penting untuk Dijelaskan:**
- Mengapa pakai watchdog? → Mencegah system hang
- Bagaimana flow kontrol akses? → RFID → Session → Access granted
- Algoritma deteksi stabilitas? → Moving average + variance check

#### B. **SensorReader - Inti Pembacaan Timbangan**
```cpp
WeightData getAdvancedWeightData() {
    // 1. Baca raw data dari HX711
    // 2. Apply moving average filter
    // 3. Apply median filter (hilangkan outlier)
    // 4. Deteksi motion/stabilitas
    // 5. Return structured data
}
```

**Algoritma Filtering:**
- **Moving Average**: Mengurangi noise random
- **Median Filter**: Menghilangkan spike/outlier
- **Stability Detection**: Variance-based algorithm

#### C. **RFIDReader - Kontrol Akses**
```cpp
bool processRFIDTag(String uid) {
    // 1. Validasi format UID
    // 2. Check authorization (local cache + Firebase)
    // 3. Grant/deny access
    // 4. Start/extend session
}
```

**Security Features:**
- UID validation (8 karakter hex)
- Local cache untuk offline access
- Session timeout (5 menit)
- Firebase sync untuk user management

#### D. **WebServer - Remote Interface**
```cpp
// REST API Endpoints:
GET  /status        → Real-time data
GET  /api/config    → Configuration
POST /api/calibrate → Kalibrasi remote
POST /api/tare      → Tare remote
GET  /api/rfid/users → User management
```

**Web Interface Features:**
- Single-page application (SPA)
- Real-time updates (1-2 detik)
- Tabbed interface (Dashboard, Config, Users)
- Responsive design

### 3. ALGORITMA TEKNIS PENTING

#### A. **Deteksi Stabilitas Berat**
```cpp
// Kriteria stabilitas:
1. Variance < threshold (0.02 kg)
2. Duration > 3 detik stabil
3. Weight > minimum (0.2 kg)
4. No motion detected
```

#### B. **Base Weight Correction**
```cpp
finalWeight = rawWeight - baseWeight;
// Untuk mengurangi berat wadah/container
```

#### C. **Session Management**
```cpp
// Auto-logout setelah 5 menit tidak aktif
// Extend session saat ada aktivitas timbangan
// Track user untuk logging dan audit
```

### 4. TEKNOLOGI YANG DIGUNAKAN

#### A. **Hardware**
- **ESP32**: Mikrokontroler dengan WiFi built-in
- **HX711**: ADC 24-bit untuk load cell
- **MFRC522**: RFID reader 13.56MHz
- **LCD I2C**: Display 16x2 atau 20x4
- **LED RGB + Buzzer**: Feedback visual/audio

#### B. **Software/Library**
- **PlatformIO**: Development environment
- **Firebase ESP Client**: Cloud database
- **AsyncWebServer**: Non-blocking web server
- **ArduinoJson**: JSON parsing
- **SPIFFS**: File system untuk web files

#### C. **Cloud Services**
- **Firebase Realtime Database**: Data storage
- **Firebase Authentication**: User management
- **NTP Server**: Time synchronization

### 5. FITUR UNGGULAN SISTEM

#### A. **Real-time Monitoring**
- Update berat setiap 200ms
- Web interface update setiap 1-2 detik
- Firebase sync otomatis saat data stabil

#### B. **Access Control**
- RFID-based authentication
- Session management dengan timeout
- User database di Firebase

#### C. **Web Configuration**
- Remote calibration
- User management
- System monitoring
- Configuration backup

#### D. **Data Quality**
- Advanced filtering (moving average + median)
- Motion detection
- Stability validation
- Quality indicators ("stable", "motion", "error")

### 6. OPTIMASI PERFORMA

#### A. **Memory Management**
- RAM usage: 15.4% (50KB/327KB)
- Flash usage: 40.1% (1.26MB/3.14MB)
- AsyncTCP stack: 8192 bytes (optimized)

#### B. **CPU Optimization**
- Configurable loop delays
- Watchdog feeding
- Task yielding
- Frequency-based updates

### 7. PERTANYAAN ANTISIPASI SIDANG

**Q: Mengapa pilih ESP32?**
A: Built-in WiFi, memory cukup, support library lengkap, cost-effective untuk IoT

**Q: Bagaimana akurasi timbangan?**
A: Tergantung load cell, ada kalibrasi, filtering advanced, deteksi stabilitas

**Q: Keamanan sistem?**
A: RFID access control, Firebase security rules, session timeout, input validation

**Q: Bagaimana handling offline?**
A: Local cache RFID users, EEPROM storage, web interface tetap bisa diakses

**Q: Scalability sistem?**
A: Modular architecture, REST API, cloud database, bisa multi-device

**Q: Error handling?**
A: Watchdog timer, try-catch blocks, status indicators, serial debugging

### 8. DEMO YANG BISA DITUNJUKKAN

1. **Hardware Demo**: Timbang objek, scan RFID, lihat LCD
2. **Web Interface**: Real-time monitoring di browser
3. **Configuration**: Ubah setting, kalibrasi remote
4. **User Management**: Tambah/hapus user RFID
5. **Firebase Integration**: Data tersimpan di cloud
6. **Access Control**: Demo authorized/unauthorized

### 9. KODE PENTING YANG HARUS DIPAHAMI

#### A. **Loop Utama (main.cpp:loop)**
- Watchdog management
- RFID access control
- Weight processing
- Stability detection
- Firebase sending

#### B. **Weight Processing (SensorReader.cpp)**
- Raw data reading
- Filtering algorithms
- Quality assessment
- Motion detection

#### C. **Web API (WebServerMicroservice.cpp)**
- REST endpoints
- JSON responses
- Configuration handling
- User management

#### D. **Session Management (SessionManager.cpp)**
- Login/logout logic
- Timeout handling
- Activity tracking

**TIPS PRESENTASI:**
- Fokus pada implementasi yang sudah jadi
- Tunjukkan kode yang berfungsi
- Jelaskan algoritma dengan diagram
- Demo real-time untuk impress
- Siap jawab pertanyaan teknis detail