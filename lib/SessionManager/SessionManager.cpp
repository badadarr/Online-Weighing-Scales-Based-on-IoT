#include "SessionManager.h"
#include "Indicator.h"
#include "lcd_display.h"
#include "WebServerMicroservice.h"

// Global instance
SessionManager sessionManager;

SessionManager::SessionManager() {
    sessionActive = false;
    currentUserUID = "";
    lastActivityTime = 0;
    sessionStartTime = 0;
}

bool SessionManager::login(String uid) {
    if (uid.length() < 4) {
        return false; // Invalid UID
    }
    
    sessionActive = true;
    currentUserUID = uid;
    lastActivityTime = millis();
    sessionStartTime = millis();
    
    // Enable data sending
    extern bool sendingActive;
    sendingActive = true;
    
    // Visual and audio feedback
    setColor(0, 255, 0); // Green - logged in
    buzz(SESSION_LOGIN_SOUND);
    
    Serial.println("[SESSION] User logged in: " + uid);
    Serial.println("[SESSION] Data sending enabled");
    lcdShowStatus("Login: " + uid.substring(0, 8) + "...");
    
    // Update web server session status
    extern TimbangangMicroserviceClient webMicroservice;
    webMicroservice.setSessionStatus(true, uid);
    
    return true;
}

bool SessionManager::logout() {
    if (!sessionActive) {
        return false; // Already logged out
    }
    
    unsigned long duration = getSessionDuration();
    String uid = currentUserUID;
    
    sessionActive = false;
    currentUserUID = "";
    lastActivityTime = 0;
    sessionStartTime = 0;
    
    // Disable data sending
    extern bool sendingActive;
    sendingActive = false;
    
    // Visual and audio feedback
    setColor(255, 0, 0); // Red - logged out
    buzz(SESSION_LOGOUT_SOUND);
    
    Serial.println("[SESSION] User logged out: " + uid);
    Serial.println("[SESSION] Session duration: " + String(duration / 1000) + " seconds");
    Serial.println("[SESSION] Data sending disabled");
    lcdShowStatus("Logout: " + uid.substring(0, 8) + "...");
    
    // Update web server session status
    extern TimbangangMicroserviceClient webMicroservice;
    webMicroservice.setSessionStatus(false, "");
    
    return true;
}

bool SessionManager::isSessionActive() {
    return sessionActive;
}

bool SessionManager::checkSessionTimeout() {
    if (!sessionActive) {
        return false; // No active session to timeout
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastActivityTime >= SESSION_TIMEOUT_MS) {
        Serial.println("[SESSION] Session timeout - auto logout");
        logout();
        lcdShowStatus("Auto Logout");
        return true;
    }
    
    return false;
}

void SessionManager::updateActivity() {
    if (sessionActive) {
        lastActivityTime = millis();
    }
}

String SessionManager::getCurrentUserUID() {
    return currentUserUID;
}

unsigned long SessionManager::getSessionDuration() {
    if (!sessionActive || sessionStartTime == 0) {
        return 0;
    }
    
    return millis() - sessionStartTime;
}

void SessionManager::printSessionStatus() {
    Serial.println("\n=== SESSION STATUS ===");
    Serial.println("Active: " + String(sessionActive ? "Yes" : "No"));
    Serial.println("User UID: " + (currentUserUID.isEmpty() ? "None" : currentUserUID));
    
    if (sessionActive) {
        Serial.println("Session Duration: " + String(getSessionDuration() / 1000) + " seconds");
        Serial.println("Idle Time: " + String((millis() - lastActivityTime) / 1000) + " seconds");
        Serial.println("Auto Logout In: " + String((SESSION_TIMEOUT_MS - (millis() - lastActivityTime)) / 1000) + " seconds");
    }
    
    Serial.println("=====================");
}