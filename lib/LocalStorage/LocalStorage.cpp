#include "LocalStorage.h"
#include <EEPROM.h>
#include "pinManager.h" // Pastikan include ini ada
#include <Arduino.h>
#include <string.h>
// File: lib/LocalStorage/LocalStorage.cpp

// Inisialisasi EEPROM
void initEEPROM()
{
  EEPROM.begin(EEPROM_SIZE);
}

bool isUIDStored(String uid)
{
  for (int i = EEPROM_ADDR_UID; i < EEPROM_SIZE; i += 32)
  {
    String stored = "";
    for (int j = 0; j < 32; j++)
    {
      char c = EEPROM.read(i + j);
      if (c == 0 || c == 255)
        break;
      stored += c;
    }
    if (stored == uid)
      return true;
  }
  return false;
}

void storeUID(String uid)
{
  if (isUIDStored(uid))
    return;
  for (int i = EEPROM_ADDR_UID; i < EEPROM_SIZE; i += 32)
  {
    if (EEPROM.read(i) == 0 || EEPROM.read(i) == 255)
    {
      for (int j = 0; j < uid.length() && j < 32; j++)
      {
        EEPROM.write(i + j, uid[j]);
      }
      EEPROM.commit();
      break;
    }
  }
}

String getAllStoredUIDs()
{
  String json = "[";
  bool first = true;

  for (int i = EEPROM_ADDR_UID; i < EEPROM_SIZE; i += 32)
  {
    String stored = "";
    for (int j = 0; j < 32; j++)
    {
      char c = EEPROM.read(i + j);
      if (c == 0 || c == 255)
        break;
      stored += c;
    }

    if (stored.length() > 0)
    {
      if (!first)
        json += ",";
      json += "{\"uid\":\"" + stored + "\",\"name\":\"User " + stored.substring(0, 4) + "\",\"email\":\"\"}";
      first = false;
    }
  }

  json += "]";
  return json;
}

// Simpan faktor kalibrasi ke EEPROM
void saveCalibrationToEEPROM(float faktor)
{
  EEPROM.put(EEPROM_ADDR_KALIBRASI, faktor);
  EEPROM.commit();
}

// Muat faktor kalibrasi dari EEPROM
float loadCalibrationFromEEPROM()
{
  float faktor = DEFAULT_FAKTOR_KALIBRASI; // Default jika EEPROM kosong
  EEPROM.get(EEPROM_ADDR_KALIBRASI, faktor);
  return faktor;
}

// Hapus semua UID dari EEPROM
void clearAllUIDs()
{
  // Asumsikan setiap slot UID 32 byte mulai dari EEPROM_ADDR_UID hingga EEPROM_SIZE
  for (int i = EEPROM_ADDR_UID; i < EEPROM_SIZE; i += 32)
  {
    for (int j = 0; j < 32; j++)
    {
      EEPROM.write(i + j, 0xFF); // tandai kosong
    }
  }
  EEPROM.commit();
}
