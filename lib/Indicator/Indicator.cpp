#include "Indicator.h"
#include "pinManager.h"

void setupIndicators()
{
  pinMode(LED_PIN_R, OUTPUT);
  pinMode(LED_PIN_G, OUTPUT);
  pinMode(LED_PIN_B, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  setColor(0, 0, 0);
  digitalWrite(BUZZER_PIN, LOW); // Matikan buzzer
  Serial.println("[INDICATOR] Inisialisasi indikator LED dan buzzer...");
}

void setColor(uint8_t r, uint8_t g, uint8_t b)
{ // Set warna LED RGB
  analogWrite(LED_PIN_R, 255 - r);
  analogWrite(LED_PIN_G, 255 - g);
  analogWrite(LED_PIN_B, 255 - b);
}

void buzz(int duration)
{ // Fungsi untuk mengaktifkan buzzer
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void LEDBuzz(int duration)
{                      // Fungsi untuk mengaktifkan buzzer
  setColor(255, 0, 0); // Merah
  delay(duration);
  setColor(0, 255, 0); // Matikan LED
  delay(duration);
  setColor(0, 0, 255); // Matikan LED
  delay(duration);
}

int ulangiBuzzer()
{
  int i;
  for (i = 0; i < 5; i++)
  {
    LEDBuzz(50); // LED dan buzzer menyala sebagai tanda siap
    buzz(50);    // Bunyi buzzer sebagai tanda siap
    delay(200);  // Jeda antar iterasi
  }
  return i; // Mengembalikan jumlah pengulangan (5)
}
