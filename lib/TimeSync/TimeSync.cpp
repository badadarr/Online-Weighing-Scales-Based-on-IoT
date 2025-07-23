#include "TimeSync.h"
#include "config.h"
#include <time.h>
#include <WiFi.h>
#include "lcd_display.h"
#include "Indicator.h"

void syncTime()
{
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  struct tm timeinfo;
  Serial.print("[NTP] Sinkronisasi waktu...");
  while (!getLocalTime(&timeinfo))
  {
    Serial.print(".");
    lcdShowError("NTP Gagal!");
    setColor(255, 0, 0); // Merah jika gagal
    buzz(2000);          // Bunyi buzzer sebagai tanda gagal
    delay(500);
  }
  Serial.println("\n[NTP] Waktu sinkron." + String(asctime(&timeinfo)));
}

String getCurrentTimestamp()
{
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
  }
  return "N/A";
}
