# USER GUIDE MANUAL
## Sistem Timbangan IoT dengan Kontrol Akses RFID

---

## 📋 DAFTAR ISI
1. [Getting Started](#getting-started)
2. [Menggunakan Timbangan](#menggunakan-timbangan)
3. [Web Interface](#web-interface)
4. [RFID User Management](#rfid-user-management)
5. [Konfigurasi Sistem](#konfigurasi-sistem)
6. [Troubleshooting](#troubleshooting)
7. [FAQ](#faq)

---

## 🚀 GETTING STARTED

### Selamat Datang!
Sistem Timbangan IoT ini dirancang untuk memberikan pengalaman penimbangan yang akurat, aman, dan mudah digunakan dengan kontrol akses berbasis RFID.

### Fitur Utama
- ✅ **Kontrol Akses RFID**: Hanya pengguna terdaftar yang dapat menggunakan
- ✅ **Real-time Monitoring**: Pantau berat secara langsung
- ✅ **Web Interface**: Akses dan konfigurasi via browser
- ✅ **Cloud Storage**: Data tersimpan otomatis di Firebase
- ✅ **Multi-user Support**: Kelola banyak pengguna RFID
- ✅ **Auto Calibration**: Kalibrasi mudah dan akurat

### Komponen Sistem

#### Hardware
- **Timbangan Digital**: Load cell dengan akurasi tinggi
- **RFID Reader**: Untuk kontrol akses pengguna
- **LCD Display**: Menampilkan berat dan status
- **LED Indicator**: Feedback visual (merah/kuning/hijau)
- **Buzzer**: Feedback audio untuk berbagai kondisi

#### Software
- **Web Dashboard**: Interface untuk monitoring dan konfigurasi
- **Firebase Database**: Penyimpanan data cloud
- **Mobile Responsive**: Dapat diakses dari smartphone

---

## ⚖️ MENGGUNAKAN TIMBANGAN

### Langkah-langkah Penggunaan

#### 1. **Power On**
- Pastikan sistem terhubung ke power supply
- Tunggu hingga LCD menampilkan "Siap digunakan"
- LED akan menyala hijau menandakan sistem ready

#### 2. **Login dengan RFID**
- Dekatkan kartu RFID ke reader (jarak <5cm)
- Tunggu bunyi beep dan LED berubah warna
- LCD akan menampilkan nama pengguna jika berhasil

#### 3. **Menimbang Objek**
- Letakkan objek di atas timbangan
- Tunggu hingga pembacaan stabil (LED hijau)
- Berat akan ditampilkan di LCD dan tersimpan otomatis

#### 4. **Logout**
- Scan kartu RFID yang sama untuk logout
- Atau tunggu 5 menit untuk auto-logout
- Sistem akan kembali ke mode standby

### Status Indikator

#### LED Colors
| Warna | Status | Keterangan |
|-------|--------|------------|
| 🔴 **Merah** | Error/Unauthorized | Kartu tidak terdaftar atau error sistem |
| 🟡 **Kuning** | Motion/Waiting | Objek bergerak atau menunggu akses |
| 🟢 **Hijau** | Stable/Ready | Pembacaan stabil atau sistem siap |
| 🔵 **Biru** | Stabilizing | Menunggu pembacaan stabil |
| 🟠 **Orange** | Calibrating | Proses kalibrasi berlangsung |

#### Buzzer Sounds
| Durasi | Kondisi | Keterangan |
|--------|---------|------------|
| 1 beep pendek | Login berhasil | Akses diberikan |
| 2 beep pendek | Logout berhasil | Sesi berakhir |
| 1 beep panjang | Data tersimpan | Berat stabil tersimpan |
| 3 beep pendek | Error | Kartu tidak valid atau error |
| Beep berulang | Motion detected | Objek masih bergerak |

#### LCD Messages
| Pesan | Keterangan |
|-------|------------|
| "Tap RFID untuk Akses" | Sistem menunggu login |
| "Access Granted - [Nama]" | Login berhasil |
| "Siap Menimbang" | Sistem siap digunakan |
| "Motion" | Objek bergerak, tunggu stabil |
| "Stable" | Pembacaan stabil |
| "Sent OK" | Data berhasil dikirim |
| "No Session" | Tidak ada sesi aktif |

---

## 🌐 WEB INTERFACE

### Mengakses Web Interface

1. **Cari IP Address**
   - Lihat di LCD display atau serial monitor
   - Biasanya format: `192.168.x.x`

2. **Buka Browser**
   - Ketik IP address di address bar
   - Contoh: `http://192.168.1.100`

3. **Interface Utama**
   - Dashboard dengan 4 tab utama
   - Real-time updates setiap 1-2 detik

### Dashboard Tab

#### Real-time Monitoring
- **Current Weight**: Berat saat ini dalam kg
- **Status**: Status koneksi sistem
- **Scale Configuration**: 
  - Base Mode: ON/OFF
  - Base Weight: Berat wadah yang dikurangi
  - Scale Ready: Status kesiapan sistem

#### User Access Information
- **Current User**: Pengguna yang sedang login
- **Session Status**: Active/Inactive
- **Total Users**: Jumlah pengguna terdaftar

#### Quick Actions
- **Quick Tare**: Reset timbangan ke nol

### Scale Config Tab

#### Konfigurasi Timbangan
- **Base Weight Mode**: 
  - ☑️ Enable: Kurangi berat wadah otomatis
  - ☐ Disable: Tampilkan berat total
- **Base Weight**: Masukkan berat wadah dalam kg
- **Calibration Factor**: Faktor kalibrasi (biasanya 1.0)
- **Stabilization Time**: Waktu tunggu stabil (detik)
- **Weight Threshold**: Ambang batas perubahan berat

#### Actions
- **Save Scale Configuration**: Simpan pengaturan
- **Tare Scale (Zero)**: Reset ke nol
- **Calibrate Scale**: Kalibrasi dengan berat standar

### System Config Tab

#### Pengaturan Sistem
- **Firebase Server URL**: URL database cloud
- **WiFi SSID**: Nama jaringan WiFi (read-only)
- **Session Timeout**: Waktu auto-logout (menit)

#### Actions
- **Save System Configuration**: Simpan pengaturan
- **Reset to Defaults**: Kembalikan ke pengaturan awal

### RFID Users Tab

#### Manajemen Pengguna
- **Add New User**:
  - RFID UID: Masukkan 8 karakter hex
  - User Name: Nama pengguna
  - Email: Email pengguna (opsional)

#### User List
- Daftar semua pengguna terdaftar
- Menampilkan nama dan UID
- **Refresh Users**: Update daftar pengguna
- **Sync RFID Data**: Sinkronisasi dengan Firebase

---

## 👥 RFID USER MANAGEMENT

### Menambah Pengguna Baru

#### Via Web Interface
1. Buka tab "RFID Users"
2. Scan kartu RFID untuk mendapatkan UID
3. Masukkan UID di field "RFID UID"
4. Masukkan nama pengguna
5. Masukkan email (opsional)
6. Klik "Add User"

#### Via Serial Monitor
```
1. Buka Serial Monitor (115200 baud)
2. Scan kartu RFID
3. Catat UID yang muncul
4. Tambahkan via web interface
```

### Format UID RFID
- **Length**: 8 karakter
- **Format**: Hexadecimal (0-9, A-F)
- **Contoh**: `A1B2C3D4`, `12345678`
- **Case**: Tidak case-sensitive (otomatis uppercase)

### Mengelola Pengguna

#### Melihat Daftar Pengguna
- Buka tab "RFID Users"
- Klik "Refresh Users" untuk update terbaru
- Daftar menampilkan nama dan UID

#### Sinkronisasi Data
- Klik "Sync RFID Data" untuk update dari Firebase
- Data akan di-cache lokal untuk akses offline
- Sinkronisasi otomatis setiap startup

### Troubleshooting RFID

#### Kartu Tidak Terdeteksi
- Pastikan jarak <5cm dari reader
- Coba orientasi kartu yang berbeda
- Pastikan kartu tidak rusak

#### UID Tidak Valid
- Pastikan format 8 karakter hex
- Contoh valid: `A1B2C3D4`
- Contoh tidak valid: `A1B2C3D4E` (terlalu panjang)

---

## ⚙️ KONFIGURASI SISTEM

### Kalibrasi Timbangan

#### Kalibrasi via Web Interface
1. Buka tab "Scale Config"
2. Pastikan timbangan kosong
3. Klik "Tare Scale (Zero)"
4. Letakkan berat standar (misal 1kg)
5. Klik "Calibrate Scale"
6. Masukkan berat standar: `1.0`
7. Tunggu proses selesai
8. Angkat berat standar

#### Kalibrasi via Serial Monitor
```
1. Buka Serial Monitor (115200 baud)
2. Ketik: kalibrasi 1.0
3. Ikuti instruksi step-by-step
4. Ketik: ok untuk konfirmasi setiap step
```

### Base Weight Configuration

#### Kapan Menggunakan Base Weight?
- Saat menggunakan wadah/container
- Untuk menimbang isi tanpa wadah
- Konsistensi dalam pengukuran

#### Cara Setting Base Weight
1. Letakkan wadah kosong di timbangan
2. Tunggu hingga stabil
3. Catat berat wadah
4. Masukkan nilai di "Base Weight"
5. Enable "Base Weight Mode"
6. Save configuration

### Network Configuration

#### WiFi Settings
- SSID dan password dikonfigurasi di `config.h`
- Sistem otomatis connect saat startup
- Support 2.4GHz WiFi networks

#### Firebase Settings
- Database URL dan API key di `config.h`
- Otomatis sync data saat stabil
- Offline mode dengan local cache

---

## 🔧 TROUBLESHOOTING

### Masalah Umum

#### Timbangan Tidak Akurat
**Gejala**: Pembacaan tidak sesuai berat sebenarnya
**Solusi**:
1. Lakukan kalibrasi ulang dengan berat standar
2. Pastikan timbangan di permukaan rata dan stabil
3. Cek tidak ada getaran atau angin
4. Periksa kondisi load cell

#### RFID Tidak Berfungsi
**Gejala**: Kartu tidak terdeteksi
**Solusi**:
1. Cek jarak kartu ke reader (<5cm)
2. Pastikan kartu tidak rusak
3. Coba kartu RFID lain
4. Restart sistem

#### Web Interface Tidak Bisa Diakses
**Gejala**: Browser tidak bisa membuka halaman
**Solusi**:
1. Cek IP address di LCD atau serial monitor
2. Pastikan komputer dan ESP32 di network yang sama
3. Coba refresh browser atau clear cache
4. Restart ESP32

#### Data Tidak Tersimpan
**Gejala**: Berat tidak muncul di Firebase
**Solusi**:
1. Cek koneksi internet
2. Verifikasi konfigurasi Firebase
3. Pastikan ada sesi RFID aktif
4. Tunggu hingga pembacaan stabil

### Error Messages

| Error Message | Penyebab | Solusi |
|---------------|----------|--------|
| "Connection Error" | WiFi terputus | Cek koneksi WiFi |
| "Firebase Error" | Masalah cloud | Cek internet dan Firebase config |
| "Sensor Error" | Masalah HX711 | Cek wiring load cell |
| "RFID Error" | Masalah MFRC522 | Cek wiring RFID reader |
| "Unauthorized" | Kartu tidak terdaftar | Tambahkan kartu ke database |

### Status Codes

#### HTTP Status Codes (Web API)
- **200**: Success
- **400**: Bad Request (parameter salah)
- **404**: Not Found (endpoint tidak ada)
- **500**: Internal Server Error

#### System Status
- **Ready**: Sistem siap digunakan
- **Busy**: Sistem sedang memproses
- **Error**: Ada masalah yang perlu diperbaiki
- **Offline**: Tidak terhubung ke network

---

## ❓ FAQ (Frequently Asked Questions)

### Penggunaan Umum

**Q: Berapa lama sistem bisa digunakan tanpa internet?**
A: Sistem dapat berfungsi offline untuk kontrol akses (menggunakan cache lokal), namun data tidak akan tersimpan ke Firebase hingga koneksi pulih.

**Q: Apakah bisa menggunakan multiple kartu RFID untuk satu user?**
A: Tidak, setiap kartu RFID hanya bisa terdaftar untuk satu user. Namun satu user bisa memiliki multiple kartu dengan UID berbeda.

**Q: Bagaimana cara backup data?**
A: Data tersimpan otomatis di Firebase. Anda bisa export data dari Firebase Console untuk backup.

**Q: Apakah sistem bisa diakses dari internet?**
A: Secara default hanya bisa diakses dari jaringan lokal. Untuk akses internet perlu konfigurasi port forwarding di router.

### Teknis

**Q: Berapa akurasi maksimum timbangan?**
A: Akurasi tergantung load cell yang digunakan. Dengan load cell 5kg, akurasi bisa mencapai ±5-10 gram setelah kalibrasi proper.

**Q: Apakah bisa menggunakan load cell dengan kapasitas berbeda?**
A: Ya, sistem mendukung berbagai kapasitas load cell. Perlu kalibrasi ulang saat mengganti load cell.

**Q: Bagaimana cara update firmware?**
A: Saat ini via USB dengan PlatformIO. OTA (Over-The-Air) update bisa diimplementasi untuk kemudahan.

**Q: Apakah data aman?**
A: Ya, menggunakan Firebase Authentication dan Security Rules. Koneksi menggunakan HTTPS/SSL.

### Maintenance

**Q: Seberapa sering perlu kalibrasi?**
A: Untuk penggunaan normal, kalibrasi bulanan sudah cukup. Untuk penggunaan intensif atau akurasi tinggi, bisa mingguan.

**Q: Bagaimana cara membersihkan sistem?**
A: Matikan power, bersihkan dengan kain lembab. Hindari air langsung ke komponen elektronik.

**Q: Apa yang harus dilakukan jika sistem hang?**
A: Tekan tombol reset pada ESP32 atau cabut-pasang power supply. Sistem akan restart otomatis.

**Q: Bagaimana cara mengganti password WiFi?**
A: Edit file `config.h`, build ulang, dan upload firmware baru ke ESP32.

---

## 📞 BANTUAN & SUPPORT

### Self-Help Resources
- **Serial Monitor**: Gunakan untuk debugging (115200 baud)
- **Web Interface**: Cek status sistem di dashboard
- **LED Indicators**: Perhatikan warna LED untuk diagnosis
- **Documentation**: Baca INSTALLATION_MANUAL.md untuk setup

### Commands Reference

#### Serial Monitor Commands
```
help              - Tampilkan semua perintah
status            - Status lengkap sistem  
test              - Test pembacaan sensor 10x
kalibrasi 1.0     - Kalibrasi dengan berat 1kg
tare              - Reset timbangan ke nol
stop              - Logout dan hentikan pengiriman
session           - Status sesi saat ini
```

#### Web API Endpoints
```
GET  /status           - Status sistem real-time
GET  /api/config       - Konfigurasi timbangan
POST /api/calibrate    - Kalibrasi remote
POST /api/tare         - Tare remote
GET  /api/rfid/users   - Daftar pengguna RFID
POST /api/add-rfid-user - Tambah pengguna baru
```

### Contact Information
- **Technical Support**: [Your Email]
- **Documentation**: GitHub Repository
- **Bug Reports**: GitHub Issues

---

## 📝 CHANGELOG & UPDATES

### Version 1.2.0 (Current)
- ✅ Web interface dengan tabbed design
- ✅ Real-time monitoring dashboard
- ✅ RFID user management
- ✅ Advanced filtering algorithms
- ✅ Session management dengan timeout
- ✅ Firebase integration
- ✅ Base weight correction
- ✅ Responsive design untuk mobile

### Planned Features
- 🔄 OTA firmware updates
- 🔄 Data export functionality
- 🔄 Email notifications
- 🔄 Multi-language support
- 🔄 Advanced reporting

---

## 🎯 TIPS & BEST PRACTICES

### Untuk Akurasi Maksimal
1. **Kalibrasi Berkala**: Lakukan kalibrasi minimal sebulan sekali
2. **Lingkungan Stabil**: Hindari getaran dan angin
3. **Berat Standar**: Gunakan berat standar yang akurat untuk kalibrasi
4. **Permukaan Rata**: Pastikan timbangan di permukaan yang rata dan stabil

### Untuk Keamanan Data
1. **Backup Rutin**: Export data Firebase secara berkala
2. **Password Strong**: Gunakan password yang kuat untuk WiFi dan Firebase
3. **Update Firmware**: Update firmware saat ada perbaikan keamanan
4. **Monitor Access**: Pantau log akses pengguna secara berkala

### Untuk Efisiensi Penggunaan
1. **Session Management**: Logout setelah selesai untuk menghemat resource
2. **Batch Weighing**: Timbang beberapa item sekaligus jika memungkinkan
3. **Regular Maintenance**: Bersihkan sensor dan cek koneksi secara berkala
4. **User Training**: Latih pengguna untuk prosedur yang benar

---

**🎉 SELAMAT MENGGUNAKAN SISTEM TIMBANGAN IoT!**

Sistem ini dirancang untuk memberikan pengalaman penimbangan yang akurat, aman, dan mudah digunakan. Jika ada pertanyaan atau masalah, jangan ragu untuk menghubungi technical support.

---

*Manual ini akan terus diupdate seiring dengan pengembangan sistem. Pastikan selalu menggunakan versi terbaru.*