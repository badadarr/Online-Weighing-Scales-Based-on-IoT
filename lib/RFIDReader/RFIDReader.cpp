#include <MFRC522.h> // Include MFRC522 library for RFID reader
#include <SPI.h> // Include SPI library for RFID communication
#include "config.h"
#include "RFIDReader.h"
//#include <FirebaseESP32.h>
#include "pinManager.h"
#include "FirebaseClient.h"
#include "LocalStorage.h"
#include "Indicator.h"
#include "lcd_display.h"

MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
extern FirebaseData fbdo;

void setupRFID() {
  Serial.begin(115200);  
  SPI.begin();
  rfid.PCD_Init();
  delay(50); // beri waktu modul siap
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);

  Serial.println("[RFID] Inisialisasi MFRC522...");
  lcdShowStatus("Inis RFID...");
   if (!rfid.PCD_PerformSelfTest()) {
    Serial.println("RFID self-test failed");
    lcdShowError("RFID Gagal! Restart...");
  } else {
    Serial.println("RFID initialized successfully");
    lcdShowStatus("RFID Siap!");
  }
  rfid.PCD_DumpVersionToSerial(); // Dump versi RFID ke Serial untuk debugging
  

}

bool isRFIDValid(String &uid) 
{ // Fungsi untuk membaca UID RFID
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return false;
  uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0"; // padding nol depan
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase(); // Ubah menjadi huruf kapital

  // Debugging output // untuk Debuging memastikan hasil scan format UID RFID
  uid.trim();
  Serial.print("[DEBUG] UID hasil scan: '");
  Serial.print(uid);
  Serial.print("' (length=");
  Serial.print(uid.length());
  Serial.println(")");

  rfid.PICC_HaltA();
  return true;
}

// Cek apakah UID sudah terdaftar di EEPROM lokal debuging
bool isUIDRegistered(String uid) { // Cek apakah UID sudah terdaftar di EEPROM lokal
  uid.trim();
  String path = "/rfid_users/" + uid; // Path untuk cek UID di Firebase
  Serial.print("[DEBUG] Path cek: ");
  Serial.println(path);

  bool found = Firebase.RTDB.getJSON(&fbdo, path);
  Serial.print("[DEBUG] getJSON: ");
  Serial.println(found);
  Serial.print("[DEBUG] dataType: ");
  Serial.println(fbdo.dataType());

  if (found && fbdo.dataType() == "json") {
    Serial.println("[RFID] UID terdaftar: " + uid);
    return true;
  } else if (found) {
    Serial.println("[RFID] Data tidak dalam format JSON.");
  } else {
    Serial.println("[RFID] UID tidak ditemukan: " + uid);
    Serial.print("[Firebase Error] ");
    Serial.println(fbdo.errorReason());
  }
  return false;
}

void requestRFIDRegistration(String uid) {
  String path = "/rfid_requests/" + uid;
  FirebaseJson json;
  json.set("device_id", DEVICE_ID);
  json.set("waktu", String(__DATE__) + " " + String(__TIME__));
  Firebase.RTDB.setJSON(&fbdo, path, &json);
}
