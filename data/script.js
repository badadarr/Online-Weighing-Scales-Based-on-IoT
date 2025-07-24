// Global variables
let lastWeight = 0;
let isConnected = false;

// Initialize when page loads
document.addEventListener("DOMContentLoaded", function () {
  console.log("Page loaded");
  if (document.getElementById("weight")) {
    // Main page - start weight monitoring
    updateWeight();
    setInterval(updateWeight, 1000);
    setInterval(updateStatus, 2000);
  } else if (document.getElementById("addUserForm")) {
    // Config page - load users and setup form
    loadUsers();
    setupAddUserForm();
    loadScaleConfig();
    loadSystemConfig();
  }
});

// Weight monitoring functions
function updateWeight() {
  fetch("/weight")
    .then((response) => response.text())
    .then((data) => {
      const weightElement = document.getElementById("weight");
      if (weightElement) {
        weightElement.textContent = data + " kg";
        lastWeight = parseFloat(data);
      }
      isConnected = true;
    })
    .catch((error) => {
      console.error("Error fetching weight:", error);
      const weightElement = document.getElementById("weight");
      if (weightElement) {
        weightElement.textContent = "Connection Error";
      }
      isConnected = false;
    });
}

function updateStatus() {
  fetch("/status")
    .then((response) => response.json())
    .then((data) => {
      const statusElement = document.getElementById("status");
      const accessElement = document.getElementById("access-status");
      const rfidElement = document.getElementById("rfid-status");

      if (statusElement) {
        statusElement.textContent = isConnected ? "Connected" : "Disconnected";
        statusElement.style.color = isConnected ? "#28a745" : "#dc3545";
      }

      // Update access status
      if (accessElement && data.access_status) {
        accessElement.textContent = data.access_status;
        accessElement.className = "access-status";
        if (
          data.access_status.includes("granted") ||
          data.access_status.includes("welcome")
        ) {
          accessElement.classList.add("granted");
        } else if (
          data.access_status.includes("denied") ||
          data.access_status.includes("unauthorized")
        ) {
          accessElement.classList.add("denied");
        }
      }

      // Update RFID status
      if (rfidElement && data.rfid_status) {
        rfidElement.textContent = "RFID: " + data.rfid_status;
      }

      // Update scale configuration status
      updateScaleConfigStatus(data);
    })
    .catch((error) => {
      console.error("Error fetching status:", error);
      const statusElement = document.getElementById("status");
      if (statusElement) {
        statusElement.textContent = "Error";
        statusElement.style.color = "#dc3545";
      }
    });
}

function updateScaleConfigStatus(data) {
  const baseModeElement = document.getElementById("baseModeStatus");
  const baseWeightElement = document.getElementById("baseWeightStatus");
  const scaleReadyElement = document.getElementById("scaleReadyStatus");

  if (baseModeElement) {
    baseModeElement.textContent = data.baseMode ? "ON" : "OFF";
    baseModeElement.style.color = data.baseMode ? "#28a745" : "#6c757d";
  }

  if (baseWeightElement) {
    baseWeightElement.textContent = data.baseWeight
      ? data.baseWeight.toFixed(3) + " kg"
      : "0.000 kg";
  }

  if (scaleReadyElement) {
    scaleReadyElement.textContent = data.systemReady ? "Ready" : "Not Ready";
    scaleReadyElement.style.color = data.systemReady ? "#28a745" : "#dc3545";
  }

  // Update user access information
  updateUserAccessStatus(data);
}

function updateUserAccessStatus(data) {
  const currentUserElement = document.getElementById("currentUserName");
  const sessionStatusElement = document.getElementById("sessionStatus");
  const totalUsersElement = document.getElementById("totalUsers");

  if (currentUserElement) {
    if (data.accessGranted && data.authorizedUser) {
      currentUserElement.textContent = data.authorizedUser;
      currentUserElement.style.color = "#28a745";
    } else {
      currentUserElement.textContent = "None";
      currentUserElement.style.color = "#6c757d";
    }
  }

  if (sessionStatusElement) {
    sessionStatusElement.textContent = data.accessGranted
      ? "Active"
      : "Inactive";
    sessionStatusElement.style.color = data.accessGranted
      ? "#28a745"
      : "#dc3545";
  }

  if (totalUsersElement) {
    totalUsersElement.textContent = data.cachedUsers || "0";
    totalUsersElement.style.color = "#007bff";
  }
}

function quickTare() {
  if (
    !confirm(
      "This will set the current reading as zero. Make sure the scale is empty. Continue?"
    )
  ) {
    return;
  }

  fetch("/api/tare", {
    method: "POST",
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.status === "success") {
        alert("Scale tared successfully!");
      } else {
        alert("Error performing tare: " + (data.message || "Unknown error"));
      }
    })
    .catch((error) => {
      console.error("Error performing tare:", error);
      alert("Network error. Please try again.");
    });
}

// RFID User Management functions
function setupAddUserForm() {
  const form = document.getElementById("addUserForm");
  if (form) {
    form.addEventListener("submit", function (e) {
      e.preventDefault();
      addUser();
    });
  }
}

function addUser() {
  const uid = document.getElementById("uid").value.trim();
  const name = document.getElementById("name").value.trim();
  const responseDiv = document.getElementById("response");

  if (!uid || !name) {
    showResponse("Please fill in all fields", "error");
    return;
  }

  // Validate UID format (basic check for hex characters)
  if (!/^[0-9A-Fa-f\s]+$/.test(uid)) {
    showResponse(
      "Invalid UID format. Please use hexadecimal characters only.",
      "error"
    );
    return;
  }

  const userData = {
    uid: uid.toUpperCase().replace(/\s+/g, ""),
    name: name,
  };

  fetch("/api/rfid/users", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(userData),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.success) {
        showResponse("User added successfully!", "success");
        document.getElementById("addUserForm").reset();
        setTimeout(() => {
          loadUsers();
        }, 1000);
      } else {
        showResponse(
          "Error adding user: " + (data.message || "Unknown error"),
          "error"
        );
      }
    })
    .catch((error) => {
      console.error("Error adding user:", error);
      showResponse("Network error. Please try again.", "error");
    });
}

function loadUsers() {
  const userListDiv = document.getElementById("userList");
  if (!userListDiv) return;

  userListDiv.textContent = "Loading users...";

  fetch("/api/rfid/users")
    .then((response) => response.json())
    .then((data) => {
      if (data.success && data.users) {
        displayUsers(data.users);
      } else {
        userListDiv.innerHTML =
          '<div class="user-item">No users found or error loading users</div>';
      }
    })
    .catch((error) => {
      console.error("Error loading users:", error);
      userListDiv.innerHTML =
        '<div class="user-item">Error loading users. Please check connection.</div>';
    });
}

function displayUsers(users) {
  const userListDiv = document.getElementById("userList");
  if (!userListDiv) return;

  if (users.length === 0) {
    userListDiv.innerHTML =
      '<div class="user-item">No users registered yet</div>';
    return;
  }

  let html = "";
  users.forEach((user) => {
    html += `
            <div class="user-item">
                <div class="user-info">
                    <div class="user-name">${escapeHtml(user.name)}</div>
                    <div class="user-uid">UID: ${escapeHtml(user.uid)}</div>
                </div>
            </div>
        `;
  });

  userListDiv.innerHTML = html;
}

function showResponse(message, type) {
  const responseDiv = document.getElementById("response");
  if (responseDiv) {
    responseDiv.textContent = message;
    responseDiv.className = "response " + type;

    // Clear message after 5 seconds
    setTimeout(() => {
      responseDiv.textContent = "";
      responseDiv.className = "response";
    }, 5000);
  }
}

function escapeHtml(text) {
  const div = document.createElement("div");
  div.textContent = text;
  return div.innerHTML;
}

// Utility functions
function formatWeight(weight) {
  return parseFloat(weight).toFixed(2);
}

function refreshPage() {
  location.reload();
}

// Export functions for global access
window.loadUsers = loadUsers;
window.addUser = addUser;
window.refreshPage = refreshPage;
window.quickTare = quickTare;
window.saveScaleConfig = saveScaleConfig;
window.performTare = performTare;
window.calibrateScale = calibrateScale;
window.saveSystemConfig = saveSystemConfig;
window.resetToDefaults = resetToDefaults;

// Scale Configuration Functions
function loadScaleConfig() {
  fetch("/api/config")
    .then((response) => response.json())
    .then((data) => {
      if (data) {
        document.getElementById("baseMode").checked = data.baseMode || false;
        document.getElementById("baseWeight").value = data.baseWeight || 0;
        document.getElementById("calibrationFactor").value =
          data.calibrationFactor || 1.0;
        document.getElementById("stabilizationTime").value =
          data.stabilizationTime || 3;
        document.getElementById("weightThreshold").value =
          data.weightThreshold || 1.0;
      }
    })
    .catch((error) => {
      console.error("Error loading scale config:", error);
      showScaleResponse("Error loading scale configuration", "error");
    });
}

function saveScaleConfig() {
  const config = {
    baseMode: document.getElementById("baseMode").checked,
    baseWeight: parseFloat(document.getElementById("baseWeight").value) || 0,
    calibrationFactor:
      parseFloat(document.getElementById("calibrationFactor").value) || 1.0,
    stabilizationTime:
      parseInt(document.getElementById("stabilizationTime").value) || 3,
    weightThreshold:
      parseFloat(document.getElementById("weightThreshold").value) || 1.0,
  };

  fetch("/api/config", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(config),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.status === "success") {
        showScaleResponse("Scale configuration saved successfully!", "success");
      } else {
        showScaleResponse(
          "Error saving configuration: " + (data.message || "Unknown error"),
          "error"
        );
      }
    })
    .catch((error) => {
      console.error("Error saving scale config:", error);
      showScaleResponse("Network error. Please try again.", "error");
    });
}

function performTare() {
  if (
    !confirm(
      "This will set the current reading as zero. Make sure the scale is empty. Continue?"
    )
  ) {
    return;
  }

  fetch("/api/tare", {
    method: "POST",
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.status === "success") {
        showScaleResponse(
          "Scale tared successfully! Current reading set to zero.",
          "success"
        );
      } else {
        showScaleResponse(
          "Error performing tare: " + (data.message || "Unknown error"),
          "error"
        );
      }
    })
    .catch((error) => {
      console.error("Error performing tare:", error);
      showScaleResponse("Network error. Please try again.", "error");
    });
}

function calibrateScale() {
  const weight = prompt(
    "Place a known weight on the scale and enter its value in kg:"
  );
  if (!weight || isNaN(weight) || parseFloat(weight) <= 0) {
    showScaleResponse("Invalid weight value entered.", "error");
    return;
  }

  if (
    !confirm(
      `Calibrating with ${weight} kg. Make sure this weight is correctly placed on the scale. Continue?`
    )
  ) {
    return;
  }

  fetch("/api/calibrate", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ knownWeight: parseFloat(weight) }),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.status === "success") {
        showScaleResponse("Scale calibrated successfully!", "success");
        loadScaleConfig(); // Reload config to show updated values
      } else {
        showScaleResponse(
          "Error calibrating scale: " + (data.message || "Unknown error"),
          "error"
        );
      }
    })
    .catch((error) => {
      console.error("Error calibrating scale:", error);
      showScaleResponse("Network error. Please try again.", "error");
    });
}

function loadSystemConfig() {
  fetch("/api/system-config")
    .then((response) => response.json())
    .then((data) => {
      if (data) {
        document.getElementById("serverURL").value = data.serverURL || "";
        document.getElementById("wifiSSID").value = data.wifiSSID || "";
        document.getElementById("sessionTimeout").value =
          data.sessionTimeout || 5;
      }
    })
    .catch((error) => {
      console.error("Error loading system config:", error);
      showSystemResponse("Error loading system configuration", "error");
    });
}

function saveSystemConfig() {
  const config = {
    serverURL: document.getElementById("serverURL").value.trim(),
    wifiSSID: document.getElementById("wifiSSID").value.trim(),
    sessionTimeout:
      parseInt(document.getElementById("sessionTimeout").value) || 5,
  };

  fetch("/api/system-config", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(config),
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.status === "success") {
        showSystemResponse(
          "System configuration saved successfully!",
          "success"
        );
      } else {
        showSystemResponse(
          "Error saving configuration: " + (data.message || "Unknown error"),
          "error"
        );
      }
    })
    .catch((error) => {
      console.error("Error saving system config:", error);
      showSystemResponse("Network error. Please try again.", "error");
    });
}

function resetToDefaults() {
  if (
    !confirm(
      "This will reset all configurations to default values. This action cannot be undone. Continue?"
    )
  ) {
    return;
  }

  fetch("/api/reset-defaults", {
    method: "POST",
  })
    .then((response) => response.json())
    .then((data) => {
      if (data.status === "success") {
        showSystemResponse(
          "All configurations reset to defaults successfully!",
          "success"
        );
        setTimeout(() => {
          loadScaleConfig();
          loadSystemConfig();
        }, 1000);
      } else {
        showSystemResponse(
          "Error resetting to defaults: " + (data.message || "Unknown error"),
          "error"
        );
      }
    })
    .catch((error) => {
      console.error("Error resetting to defaults:", error);
      showSystemResponse("Network error. Please try again.", "error");
    });
}

function showScaleResponse(message, type) {
  const responseDiv = document.getElementById("scaleResponse");
  if (responseDiv) {
    responseDiv.textContent = message;
    responseDiv.className = "response " + type;

    // Clear message after 5 seconds
    setTimeout(() => {
      responseDiv.textContent = "";
      responseDiv.className = "response";
    }, 5000);
  }
}

function showSystemResponse(message, type) {
  const responseDiv = document.getElementById("systemResponse");
  if (responseDiv) {
    responseDiv.textContent = message;
    responseDiv.className = "response " + type;

    // Clear message after 5 seconds
    setTimeout(() => {
      responseDiv.textContent = "";
      responseDiv.className = "response";
    }, 5000);
  }
}

// Export new functions for global access
window.loadScaleConfig = loadScaleConfig;
window.saveScaleConfig = saveScaleConfig;
window.performTare = performTare;
window.calibrateScale = calibrateScale;
window.loadSystemConfig = loadSystemConfig;
window.saveSystemConfig = saveSystemConfig;
window.resetToDefaults = resetToDefaults;
window.quickTare = quickTare;
