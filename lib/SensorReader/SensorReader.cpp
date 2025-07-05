#include "SensorReader.h"
#include "HX711.h"
#include "pinManager.h"
#include "lcd_display.h"
#include "LocalStorage.h"
#include "Indicator.h"

HX711 scale;
bool lastTareButtonState = HIGH;
float FaktorKalibrasi = FAKTOR_KALIBRASI; // Faktor kalibrasi default

// Fungsi inisialisasi sensor (tetap dipakai)
void setupSensor() {
    pinMode(TARE_BUTTON_PIN, INPUT_PULLUP);
    scale.begin(HX711_DATA_PIN, HX711_CLOCK_PIN);
    scale.set_scale(FAKTOR_KALIBRASI);
    scale.tare();
    //lcdShowStatus("Inis Sensor...");
}

// Fungsi baca berat (tetap dipakai)
String readWeight() {
    if (scale.is_ready()) {
        float berat = scale.get_units(WEIGHT_SAMPLES);
        // filter nilai minus atau terlalu kecil
        if (berat < 0 || abs(berat) < 0.02f) { // Ganti 2.0f jadi 0.02f (20 gram)
          berat = 0.0f;
        } else if (berat > 10000) {
          berat = 10000;
        }else if(berat > 0){
            buzz(10); // Bunyi buzzer jika berat valid
        }

        Serial.print("[SCALE] Berat terbaca: ");
        Serial.println(berat, 3);
        return String(berat, 3);
    }
    return "";
}

// Fungsi tambahan: update tombol tare
void updateTareButton() {
    bool tareButtonState = digitalRead(TARE_BUTTON_PIN);
    if (lastTareButtonState == HIGH && tareButtonState == LOW) {
        Serial.println("[SCALE] Tombol tare ditekan.");
        Serial.println("[SCALE] Melakukan tare (reset ke nol)...");
        scale.tare();
        Serial.println("[SCALE] Tare selesai.");
        // Tampilkan pesan di LCD
        lcdShowTare("Tare Selesai.."); // Jika menggunakan LCD, tampil
        delay(2000);
    }
    lastTareButtonState = tareButtonState;
}

// Implementasi class Scale

void Kalibrasi(float knownWeight) {
    if (knownWeight == 0) {
        Serial.println("[SCALE] Error: Berat standar tidak boleh nol!");
        return;
    }
    Serial.println("[SCALE] Mulai kalibrasi...");
    buzz(100); // Bunyi buzzer sebagai tanda mulai kalibrasi 
    scale.set_scale();
    scale.tare();
    delay(500);
    Serial.println("[SCALE] Letakkan beban standar di atas timbangan.");
    delay(2000); // Beri waktu letakkan beban

    float reading = scale.get_units(10);
    Serial.print("[SCALE] Pembacaan rata-rata: ");
    Serial.println(reading);

    if (abs(reading) < 0.01) {
      buzz(100); // Bunyi buzzer sebagai tanda gagal
        Serial.println("[SCALE] Error: Pembacaan terlalu kecil, kalibrasi gagal!");
        return;
    }

    FaktorKalibrasi = reading / knownWeight;
    scale.set_scale(FaktorKalibrasi);
    Serial.print("[SCALE] Faktor kalibrasi baru: ");
    Serial.println(FaktorKalibrasi);

    // Simpan ke EEPROM jika perlu
    saveCalibrationToEEPROM(getFaktorKalibrasi()); // Simpan faktor kalibrasi ke EEPROM
    buzz(100); // Bunyi buzzer sebagai tanda selesai
    Serial.println("[SCALE] Kalibrasi selesai. Faktor kalibrasi disimpan ke EEPROM.");
}



float getFaktorKalibrasi() {
    return FaktorKalibrasi;
}

void setFaktorKalibrasi(float factor) {
    FaktorKalibrasi = factor;
    scale.set_scale(FaktorKalibrasi);
}
