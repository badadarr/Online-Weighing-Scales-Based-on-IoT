#pragma once
#include <Arduino.h>
#include <stdint.h>
void setupIndicators();
void setColor(uint8_t r, uint8_t g, uint8_t b);
void buzz(int duration);
void LEDBuzz(int duration); // Tambahkan fungsi LEDBuzz untuk mengaktifkan LED hijau
int ulangiBuzzer(); // Fungsi untuk mengulangi LED dan buzzer
