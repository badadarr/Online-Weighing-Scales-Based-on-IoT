#pragma once
#include <Arduino.h>

void setupRFID();
bool isRFIDValid(String &uid);
bool isUIDRegistered(String uid);
void requestRFIDRegistration(String uid);

