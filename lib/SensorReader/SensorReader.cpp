#include "SensorReader.h"
#include "HX711.h"
#include "pinManager.h"
#include "lcd_display.h"
#include "LocalStorage.h"
#include "Indicator.h"
#include "config.h"

HX711 scale;
bool lastTareButtonState = HIGH;
float FaktorKalibrasi = FAKTOR_KALIBRASI; // Faktor kalibrasi default

// Advanced filtering variables
#define WEIGHT_BUFFER_SIZE 10
#define STABILITY_THRESHOLD 0.05  // 50 grams threshold for stability
#define MOTION_THRESHOLD 0.1      // 100 grams threshold for motion detection
#define STABILITY_COUNT_REQUIRED 5 // Number of stable readings required

float weightBuffer[WEIGHT_BUFFER_SIZE];
int bufferIndex = 0;
bool bufferFilled = false;
float lastStableWeight = 0.0;
int stableCount = 0;
unsigned long lastWeightUpdate = 0;
bool motionDetected = false;

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

        // Serial.print("[SCALE] Berat terbaca: ");
        // Serial.println(berat, WEIGHT_PRECISION); // Mengurangi log berulang
        return String(berat, WEIGHT_PRECISION); // Format: 0.000 kg
    }
    return "";
}

// Fungsi tambahan: update tombol tare
void updateTareButton() {
    bool tareButtonState = digitalRead(TARE_BUTTON_PIN);
    if (lastTareButtonState == HIGH && tareButtonState == LOW) {
        Serial.println("[SCALE] Tombol tare ditekan.");
        performTare(); // Use the new advanced tare function
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
    
    // Reset scale and tare
    scale.set_scale();
    performTare(); // Use advanced tare function
    delay(1000);
    
    Serial.println("[SCALE] Letakkan beban standar di atas timbangan.");
    Serial.print("[SCALE] Berat standar: ");
    Serial.print(knownWeight);
    Serial.println(" kg");
    
    // Wait for stable reading
    delay(3000); // Give time to place weight
    
    // Take multiple readings for better accuracy
    float totalReading = 0;
    int validReadings = 0;
    
    for (int i = 0; i < 20; i++) {
        if (scale.is_ready()) {
            float reading = scale.get_units(1);
            if (abs(reading) > 0.01) { // Valid reading
                totalReading += reading;
                validReadings++;
            }
        }
        delay(100);
    }
    
    if (validReadings < 10) {
        buzz(200); // Error buzz
        Serial.println("[SCALE] Error: Tidak cukup pembacaan valid untuk kalibrasi!");
        return;
    }
    
    float averageReading = totalReading / validReadings;
    Serial.print("[SCALE] Pembacaan rata-rata dari ");
    Serial.print(validReadings);
    Serial.print(" sampel: ");
    Serial.println(averageReading);

    if (abs(averageReading) < 0.01) {
        buzz(200); // Error buzz
        Serial.println("[SCALE] Error: Pembacaan terlalu kecil, kalibrasi gagal!");
        return;
    }

    FaktorKalibrasi = averageReading / knownWeight;
    scale.set_scale(FaktorKalibrasi);
    Serial.print("[SCALE] Faktor kalibrasi baru: ");
    Serial.println(FaktorKalibrasi, 6);

    // Test the calibration
    delay(1000);
    float testReading = scale.get_units(10);
    Serial.print("[SCALE] Test pembacaan setelah kalibrasi: ");
    Serial.print(testReading, 3);
    Serial.println(" kg");
    
    float error = abs(testReading - knownWeight);
    float errorPercent = (error / knownWeight) * 100;
    
    if (errorPercent < 5.0) { // Less than 5% error
        // Simpan ke EEPROM jika berhasil
        saveCalibrationToEEPROM(getFaktorKalibrasi());
        buzz(100); // Success buzz
        Serial.print("[SCALE] Kalibrasi berhasil! Error: ");
        Serial.print(errorPercent, 2);
        Serial.println("%");
        Serial.println("[SCALE] Faktor kalibrasi disimpan ke EEPROM.");
        
        // Reset filtering after calibration
        bufferIndex = 0;
        bufferFilled = false;
        stableCount = 0;
        lastStableWeight = 0.0;
        
    } else {
        buzz(300); // Warning buzz
        Serial.print("[SCALE] Peringatan: Error kalibrasi tinggi (");
        Serial.print(errorPercent, 2);
        Serial.println("%). Periksa beban standar dan ulangi kalibrasi.");
    }
}



float getFaktorKalibrasi() {
    return FaktorKalibrasi;
}

void setFaktorKalibrasi(float factor) {
    FaktorKalibrasi = factor;
    scale.set_scale(FaktorKalibrasi);
}

// Advanced weight filtering functions
void updateWeightBuffer(float newWeight) {
    weightBuffer[bufferIndex] = newWeight;
    bufferIndex = (bufferIndex + 1) % WEIGHT_BUFFER_SIZE;
    if (!bufferFilled && bufferIndex == 0) {
        bufferFilled = true;
    }
}

float calculateMovingAverage() {
    if (!bufferFilled && bufferIndex == 0) return 0.0;
    
    float sum = 0.0;
    int count = bufferFilled ? WEIGHT_BUFFER_SIZE : bufferIndex;
    
    for (int i = 0; i < count; i++) {
        sum += weightBuffer[i];
    }
    
    return sum / count;
}

float calculateMedianFilter() {
    if (!bufferFilled && bufferIndex < 3) return 0.0;
    
    int count = bufferFilled ? WEIGHT_BUFFER_SIZE : bufferIndex;
    float tempBuffer[WEIGHT_BUFFER_SIZE];
    
    // Copy buffer for sorting
    for (int i = 0; i < count; i++) {
        tempBuffer[i] = weightBuffer[i];
    }
    
    // Simple bubble sort
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (tempBuffer[j] > tempBuffer[j + 1]) {
                float temp = tempBuffer[j];
                tempBuffer[j] = tempBuffer[j + 1];
                tempBuffer[j + 1] = temp;
            }
        }
    }
    
    // Return median
    if (count % 2 == 0) {
        return (tempBuffer[count/2 - 1] + tempBuffer[count/2]) / 2.0;
    } else {
        return tempBuffer[count/2];
    }
}

bool detectMotion(float currentWeight) {
    static float lastWeight = 0.0;
    static unsigned long lastMotionTime = 0;
    
    float weightDiff = abs(currentWeight - lastWeight);
    bool hasMotion = weightDiff > MOTION_THRESHOLD;
    
    if (hasMotion) {
        lastMotionTime = millis();
        motionDetected = true;
    } else if (millis() - lastMotionTime > 2000) { // 2 seconds of no motion
        motionDetected = false;
    }
    
    lastWeight = currentWeight;
    return motionDetected;
}

bool isWeightStable() {
    if (!bufferFilled && bufferIndex < STABILITY_COUNT_REQUIRED) return false;
    
    float filtered = calculateMedianFilter();
    float diff = abs(filtered - lastStableWeight);
    
    if (diff < STABILITY_THRESHOLD) {
        stableCount++;
        if (stableCount >= STABILITY_COUNT_REQUIRED) {
            return true;
        }
    } else {
        stableCount = 0;
        lastStableWeight = filtered;
    }
    
    return false;
}

float getFilteredWeight() {
    float median = calculateMedianFilter();
    float average = calculateMovingAverage();
    
    // Use median for better noise rejection, but fallback to average if needed
    return abs(median - average) < 0.1 ? median : average;
}

String getWeightQuality() {
    bool stable = isWeightStable();
    bool motion = motionDetected;
    
    if (motion) {
        return "motion";
    } else if (stable) {
        return "stable";
    } else {
        return "stabilizing";
    }
}

WeightData getAdvancedWeightData() {
    WeightData data;
    
    if (scale.is_ready()) {
        // Get raw reading
        data.raw = scale.get_units(1);
        
        // Update buffer with raw data
        updateWeightBuffer(data.raw);
        
        // Calculate filtered weight
        data.filtered = getFilteredWeight();
        
        // Apply basic filtering for negative/small values
        if (data.filtered < 0 || abs(data.filtered) < 0.02f) {
            data.filtered = 0.0f;
        } else if (data.filtered > 10000) {
            data.filtered = 10000;
        }
        
        // Detect motion and stability
        data.hasMotion = detectMotion(data.filtered);
        data.isStable = isWeightStable();
        data.quality = getWeightQuality();
        
        // Set stable weight (use filtered if stable, otherwise last stable value)
        if (data.isStable) {
            data.stable = data.filtered;
            lastStableWeight = data.stable;
        } else {
            data.stable = lastStableWeight;
        }
        
        data.lastUpdate = millis();
        
        // Buzzer for valid weight detection
        if (data.filtered > 0 && data.isStable) {
            buzz(10);
        }
        
    } else {
        // Scale not ready
        data.raw = 0;
        data.filtered = 0;
        data.stable = lastStableWeight;
        data.isStable = false;
        data.hasMotion = false;
        data.quality = "error";
        data.lastUpdate = millis();
    }
    
    return data;
}

void performTare() {
    Serial.println("[SCALE] Performing tare operation...");
    scale.tare();
    
    // Reset filtering variables
    bufferIndex = 0;
    bufferFilled = false;
    stableCount = 0;
    lastStableWeight = 0.0;
    motionDetected = false;
    
    Serial.println("[SCALE] Tare completed and filters reset.");
    lcdShowTare("Tare Selesai..");
    buzz(100);
}
