# INSTALLATION MANUAL
## Sistem Timbangan IoT dengan Kontrol Akses RFID

---

## 📋 DAFTAR ISI
1. [Requirements](#requirements)
2. [Hardware Installation](#hardware-installation)
3. [Software Installation](#software-installation)
4. [Firebase Setup](#firebase-setup)
5. [System Testing](#system-testing)
6. [Troubleshooting](#troubleshooting)
7. [Deployment](#deployment)

---

## 🔧 REQUIREMENTS

### Hardware Requirements
| Komponen | Spesifikasi | Jumlah |
|----------|-------------|---------|
| ESP32 Development Board | ESP32-WROOM-32 | 1 |
| Load Cell | 5kg atau 10kg | 1 |
| HX711 ADC Module | 24-bit ADC | 1 |
| MFRC522 RFID Reader | 13.56MHz | 1 |
| LCD I2C Display | 16x2 atau 20x4 | 1 |
| LED RGB | Common Cathode | 1 |
| Buzzer | Active/Passive 5V | 1 |
| Resistor | 220Ω | 3 |
| Breadboard/PCB | - | 1 |
| Jumper Wires | Male-Female | 20+ |
| Power Supply | 5V/2A | 1 |

### Software Requirements
- **PlatformIO IDE** (recommended) atau Arduino IDE
- **Git** untuk version control
- **Web Browser** (Chrome/Firefox/Edge)
- **Firebase Account** (Google account)

### Network Requirements
- **WiFi Network** dengan akses internet
- **Computer** dengan port USB

---

## ⚡ HARDWARE INSTALLATION

### Wiring Diagram

#### ESP32 ↔ HX711 (Load Cell ADC)
```
ESP32 Pin    →    HX711 Pin
GPIO 4       →    DT (Data)
GPIO 2       →    SCK (Clock)
3.3V         →    VCC
GND          →    GND
```

#### ESP32 ↔ MFRC522 (RFID Reader)
```
ESP32 Pin    →    MFRC522 Pin
GPIO 21      →    SDA
GPIO 18      →    SCK
GPIO 23      →    MOSI
GPIO 19      →    MISO
GPIO 22      →    RST
3.3V         →    3.3V
GND          →    GND
```

#### ESP32 ↔ LCD I2C
```
ESP32 Pin    →    LCD I2C Pin
GPIO 21      →    SDA
GPIO 22      →    SCL
5V           →    VCC
GND          →    GND
```

#### ESP32 ↔ LED RGB
```
ESP32 Pin    →    LED Pin
GPIO 25      →    Red (+ 220Ω resistor)
GPIO 26      →    Green (+ 220Ω resistor)
GPIO 27      →    Blue (+ 220Ω resistor)
GND          →    Common Cathode
```

#### ESP32 ↔ Buzzer
```
ESP32 Pin    →    Buzzer Pin
GPIO 33      →    Positive
GND          →    Negative
```

### Assembly Steps

1. **Persiapan Load Cell**
   - Mount load cell pada frame timbangan yang stabil
   - Pastikan load cell tidak terhalang dan dapat bergerak bebas
   - Hubungkan kabel load cell ke HX711: E+, E-, A+, A-

2. **Wiring ESP32**
   - Ikuti diagram wiring di atas dengan teliti
   - Gunakan breadboard untuk prototyping
   - Pastikan koneksi kuat dan tidak longgar

3. **Test Koneksi**
   - Gunakan multimeter untuk memverifikasi koneksi
   - Cek kontinuitas setiap jalur
   - Pastikan tidak ada short circuit

4. **Mounting**
   - Tempatkan ESP32 dalam enclosure yang aman
   - Pastikan ventilasi yang cukup
   - Amankan semua koneksi dengan solder (untuk produksi)

---

## 💻 SOFTWARE INSTALLATION

### 1. Install PlatformIO

#### Opsi A: PlatformIO Core (Command Line)
```bash
# Install Python terlebih dahulu, kemudian:
pip install platformio
```

#### Opsi B: PlatformIO IDE Extension (VS Code)
1. Install Visual Studio Code
2. Go to Extensions (Ctrl+Shift+X)
3. Search "PlatformIO IDE"
4. Click Install

### 2. Clone Project
```bash
git clone <repository-url>
cd Online-Weighing-Scales-Based-on-IoT
```

### 3. Configure System

Edit file `include/config.h`:

```cpp
// ==================== NETWORK CONFIGURATION ====================
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ==================== FIREBASE CONFIGURATION ====================
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"
#define USER_EMAIL "YOUR_FIREBASE_EMAIL"
#define USER_PASSWORD "YOUR_FIREBASE_PASSWORD"

// ==================== DEVICE IDENTIFICATION ====================
#define DEVICE_ID "esp32_timbangan_001"
```

### 4. Build & Upload

```bash
# Build project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

---

## 🔥 FIREBASE SETUP

### 1. Create Firebase Project

1. Buka [Firebase Console](https://console.firebase.google.com)
2. Klik "Create a project"
3. Masukkan nama project: `iot-scales-enhancement`
4. Enable Google Analytics (opsional)
5. Klik "Create project"

### 2. Setup Realtime Database

1. Pilih "Realtime Database" di menu kiri
2. Klik "Create Database"
3. Pilih lokasi: `asia-southeast1` (Singapore)
4. Mulai dalam **test mode** (untuk development)
5. Klik "Done"

### 3. Setup Authentication

1. Pilih "Authentication" → "Sign-in method"
2. Enable "Email/Password"
3. Tambah user baru:
   - Email: `esp32@timbangan-iot.com`
   - Password: `esp32secure123`

### 4. Configure Security Rules

Masukkan rules berikut di Realtime Database Rules:

```json
{
  "rules": {
    "current_data": {
      ".read": "auth != null",
      ".write": "auth != null"
    },
    "authorized_users": {
      ".read": "auth != null",
      ".write": "auth != null"
    },
    "rfid_users": {
      ".read": "auth != null",
      ".write": "auth != null"
    },
    "weight_history": {
      ".read": "auth != null",
      ".write": "auth != null"
    }
  }
}
```

### 5. Get Configuration Keys

1. Pilih Project Settings (ikon gear)
2. Tab "General"
3. Scroll ke "Your apps"
4. Klik ikon "Web app"
5. Copy konfigurasi ke `config.h`

---

## 🧪 SYSTEM TESTING

### 1. Hardware Test

#### Power On Test
```
1. Hubungkan ESP32 ke power supply
2. Buka Serial Monitor (115200 baud)
3. Lihat boot messages:
   [SYSTEM] Booting...
   [WiFi] Connecting...
   [Firebase] Authentication successful!
```

#### Sensor Test
```
Serial Commands:
- status    → Lihat status semua sensor
- test      → Test pembacaan 10x berturut-turut
- help      → Lihat semua perintah
```

### 2. Network Test

#### WiFi Connection
- Cek IP address di Serial Monitor
- Ping ESP32 dari computer: `ping <ESP32_IP>`

#### Web Interface
- Buka browser: `http://<ESP32_IP>`
- Cek semua tab: Dashboard, Scale Config, System Config, RFID Users

#### Firebase Connection
- Lihat data di Firebase Console
- Test real-time sync dengan menimbang objek

### 3. Calibration

#### Step-by-step Calibration
```
1. Pastikan timbangan kosong
2. Serial command: tare
3. Letakkan beban standar 1kg
4. Serial command: kalibrasi 1.0
5. Tunggu hingga selesai
6. Angkat beban
7. Test dengan berbagai berat
```

---

## 🔧 TROUBLESHOOTING

### Common Issues

#### ESP32 Won't Boot
**Symptoms:** No serial output, LED tidak menyala
**Solutions:**
- Cek power supply (minimal 5V/1A)
- Verifikasi wiring connections
- Coba USB cable yang berbeda
- Tekan tombol RESET pada ESP32

#### WiFi Connection Failed
**Symptoms:** `[WiFi] Connection failed`
**Solutions:**
- Cek SSID/password di config.h
- Pastikan network 2.4GHz (bukan 5GHz)
- Cek signal strength WiFi
- Restart router jika perlu

#### Firebase Connection Failed
**Symptoms:** `[Firebase] Authentication failed`
**Solutions:**
- Verifikasi API key dan database URL
- Cek email/password authentication
- Pastikan koneksi internet stabil
- Cek Firebase project settings

#### Sensor Reading Error
**Symptoms:** Weight selalu 0 atau nilai aneh
**Solutions:**
- Cek wiring HX711 dan load cell
- Verifikasi power supply HX711
- Test dengan multimeter
- Lakukan kalibrasi ulang

#### RFID Not Working
**Symptoms:** Tidak detect kartu RFID
**Solutions:**
- Cek wiring SPI MFRC522
- Verifikasi power supply 3.3V
- Test dengan kartu RFID berbeda
- Cek jarak kartu ke reader (<5cm)

### Debug Commands

```cpp
// Serial Monitor Commands (115200 baud):
help              // Tampilkan semua perintah
status            // Status lengkap sistem
test              // Test sensor 10x
kalibrasi 1.0     // Kalibrasi dengan berat 1kg
tare              // Reset ke nol
stop              // Logout dan stop sending
session           // Status session saat ini
```

---

## 🚀 DEPLOYMENT

### Production Setup

#### 1. Hardware Finalization
- **Enclosure**: Gunakan box tahan air IP65
- **Mounting**: Secure mounting untuk load cell
- **Power**: Regulated power supply dengan backup
- **Cables**: Gunakan kabel berkualitas dengan strain relief

#### 2. Software Configuration
- **Static IP**: Configure static IP untuk akses konsisten
- **OTA Updates**: Enable Over-The-Air updates
- **Logging**: Configure logging level untuk production
- **Security**: Ganti password default

#### 3. Network Setup
- **Port Forwarding**: Jika perlu akses dari internet
- **Firewall**: Configure firewall rules
- **VPN**: Setup VPN untuk remote access yang aman

### Maintenance Schedule

#### Daily
- Cek status sistem via web interface
- Monitor Firebase data

#### Weekly  
- Cek akurasi dengan berat standar
- Backup konfigurasi sistem

#### Monthly
- Kalibrasi ulang dengan berat standar
- Update firmware jika ada
- Cek kondisi hardware

#### Quarterly
- Inspeksi fisik semua koneksi
- Cleaning sensor dan komponen
- Backup database Firebase

---

## 📊 SYSTEM SPECIFICATIONS

### Performance
- **Weight Range**: 0-10kg (tergantung load cell)
- **Accuracy**: ±0.01kg (setelah kalibrasi)
- **Response Time**: <1 detik
- **Stability Time**: 3 detik
- **Update Rate**: 200ms (sensor), 1-2s (web)

### Power Consumption
- **Active Mode**: ~200mA @ 5V
- **Standby**: ~150mA @ 5V
- **Deep Sleep**: ~10mA @ 5V (jika diimplementasi)

### Environmental
- **Operating Temperature**: 0°C to 50°C
- **Humidity**: 10% to 90% RH
- **Storage**: -20°C to 70°C

---

## ⚠️ SAFETY WARNINGS

### Electrical Safety
- ⚠️ Selalu matikan power sebelum wiring
- ⚠️ Gunakan kabel dengan rating arus yang sesuai
- ⚠️ Pastikan grounding yang proper
- ⚠️ Jangan operasikan dalam kondisi basah

### Mechanical Safety
- ⚠️ Secure mounting load cell
- ⚠️ Jangan melebihi kapasitas maksimum
- ⚠️ Inspeksi berkala komponen mekanik
- ⚠️ Gunakan safety factor 2x untuk beban

### Data Security
- ⚠️ Ganti password default
- ⚠️ Gunakan HTTPS untuk web access
- ⚠️ Regular security updates
- ⚠️ Backup data secara berkala

---

## 📞 SUPPORT

### Documentation
- Source code comments dalam setiap file
- API documentation di web interface
- User manual untuk operasional

### Technical Support
- **GitHub Issues**: [Repository URL]
- **Email**: [Your Email]
- **Documentation**: README.md dan file manual ini

---

## ✅ INSTALLATION CHECKLIST

- [ ] Hardware assembled sesuai wiring diagram
- [ ] Power supply tested dan stabil
- [ ] ESP32 boot successfully dengan serial output
- [ ] WiFi connected dan mendapat IP address
- [ ] Firebase authentication berhasil
- [ ] Web interface accessible di browser
- [ ] Load cell memberikan reading yang masuk akal
- [ ] RFID reader dapat detect kartu
- [ ] LCD display menampilkan informasi
- [ ] LED dan buzzer berfungsi
- [ ] Kalibrasi completed dengan berat standar
- [ ] System test dengan berbagai skenario
- [ ] Documentation dan backup completed

---

**🎉 INSTALLATION COMPLETE!**

Sistem siap untuk operasional. Lanjutkan ke **USER_GUIDE_MANUAL.md** untuk panduan penggunaan.
