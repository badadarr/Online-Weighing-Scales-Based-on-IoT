#pragma once
#include <Arduino.h>

// RFID setup and basic functions
void setupRFID();
bool isRFIDValid(String &uid);

// Data collection functions
bool collectRFIDUsersData();
bool syncRFIDUsersFromFirebase();
bool isRFIDUsersDataCached();
void clearRFIDUsersCache();
int getCachedUsersCount();

// Access control functions
bool isUIDAuthorized(String uid);
void requestRFIDRegistration(String uid);
bool grantWeighingAccess(String uid);
void resetAccess();
bool isWeighingAccessGranted();
String getCurrentAuthorizedUser();
void extendAccess();
bool processRFIDTag(String uid);
void handleRFIDAccess();

// Legacy functions for compatibility
bool isUIDRegistered(String uid); // Now calls isUIDAuthorized
bool handleRFIDLogin(String uid); // Now calls grantWeighingAccess
bool handleRFIDLogout(String uid); // Now calls resetAccess

