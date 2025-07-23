#pragma once
#include <Arduino.h>
#include "config.h"

class SessionManager {
private:
    bool sessionActive;
    String currentUserUID;
    unsigned long lastActivityTime;
    unsigned long sessionStartTime;

public:
    SessionManager();
    
    // Session management
    bool login(String uid);
    bool logout();
    bool isSessionActive();
    bool checkSessionTimeout();
    void updateActivity();
    
    // User management
    String getCurrentUserUID();
    unsigned long getSessionDuration();
    
    // Session status
    void printSessionStatus();
};

// Global session manager instance
extern SessionManager sessionManager;