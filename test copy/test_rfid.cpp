#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>

// Pin MFRC522 sesuai wiring Anda
#define RFID_RST_PIN 27
#define RFID_SS_PIN 5
#define RFID_SCK_PIN 18
#define RFID_MOSI_PIN 23
#define RFID_MISO_PIN 19

// Inisialisasi MFRC522
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

byte gagalCounter = 0;
const byte batasGagal = 3;

void setup()
{
    Serial.begin(115200);

    // Matikan WiFi untuk mengurangi noise
    WiFi.mode(WIFI_OFF);
    WiFi.disconnect();

    // Inisialisasi SPI dengan pin custom
    SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);

    // Init MFRC522 dengan SPI speed rendah
    mfrc522.PCD_Init();
    mfrc522.PCD_WriteRegister(mfrc522.TModeReg, 0x8D);
    mfrc522.PCD_WriteRegister(mfrc522.TPrescalerReg, 0x3E);
    mfrc522.PCD_WriteRegister(mfrc522.TReloadRegL, 30);
    mfrc522.PCD_WriteRegister(mfrc522.TReloadRegH, 0);

    Serial.println("Siap scan kartu RFID...");
}

void loop()
{
    // Jika tidak ada kartu
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    {
        gagalCounter++;
        delay(100);

        if (gagalCounter >= batasGagal)
        {
            Serial.println(" Modul hang, reset hardware...");
            resetRFID();
            gagalCounter = 0;
        }
        return;
    }

    // Reset counter gagal jika kartu terbaca
    gagalCounter = 0;

    // Cetak UID kartu
    Serial.print("UID: ");
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();

    delay(500);
}

void resetRFID()
{
    // Reset hardware lewat pin RST
    pinMode(RFID_RST_PIN, OUTPUT);
    digitalWrite(RFID_RST_PIN, LOW);
    delay(50);
    digitalWrite(RFID_RST_PIN, HIGH);
    delay(50);

    // Re-init MFRC522
    mfrc522.PCD_Init();
}