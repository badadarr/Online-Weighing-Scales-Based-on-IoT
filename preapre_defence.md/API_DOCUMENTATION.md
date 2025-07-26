# API DOCUMENTATION
## REST API Reference - Sistem Timbangan IoT

---

## 📋 OVERVIEW

API ini menyediakan akses programmatic ke sistem timbangan IoT melalui HTTP REST endpoints. Semua response dalam format JSON.

**Base URL**: `http://<ESP32_IP>/`
**Content-Type**: `application/json`
**Method**: GET, POST

---

## 🔗 ENDPOINTS

### 1. System Status

#### GET `/status`
Mendapatkan status sistem real-time.

**Response:**
```json
{
  "weight": 1.234,
  "rawWeight": 1.235,
  "baseMode": true,
  "baseWeight": 0.100,
  "lastRFID": "A1B2C3D4",
  "systemReady": true,
  "isStable": true,
  "stabilizationStatus": "stable",
  "remainingTime": 0,
  "accessGranted": true,
  "authorizedUser": "John Doe",
  "rfidDataCached": true,
  "cachedUsers": 5,
  "serverURL": "http://192.168.1.100",
  "access_status": "Access granted - Welcome John Doe",
  "rfid_status": "5 users loaded"
}
```

#### GET `/api/status`
Alias untuk `/status` endpoint.

### 2. Configuration

#### GET `/api/config`
Mendapatkan konfigurasi timbangan saat ini.

**Response:**
```json
{
  "baseMode": true,
  "baseWeight": 0.100,
  "calibrationFactor": 1.0,
  "stabilizationTime": 3,
  "weightThreshold": 1.0,
  "serverURL": "http://192.168.1.100"
}
```

#### POST `/api/config`
Mengupdate konfigurasi timbangan.

**Request Body:**
```json
{
  "baseMode": true,
  "baseWeight": 0.100,
  "calibrationFactor": 1.0,
  "stabilizationTime": 3,
  "weightThreshold": 1.0,
  "serverURL": "http://192.168.1.100"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Configuration saved successfully"
}
```

### 3. System Configuration

#### GET `/api/system-config`
Mendapatkan konfigurasi sistem.

**Response:**
```json
{
  "serverURL": "http://192.168.1.100",
  "wifiSSID": "MyWiFi",
  "sessionTimeout": 5
}
```

#### POST `/api/system-config`
Mengupdate konfigurasi sistem.

**Request Body:**
```json
{
  "serverURL": "http://192.168.1.100",
  "sessionTimeout": 5
}
```

**Response:**
```json
{
  "status": "success",
  "message": "System configuration saved successfully"
}
```

### 4. Calibration & Tare

#### POST `/api/calibrate`
Melakukan kalibrasi dengan berat standar.

**Request Body:**
```json
{
  "knownWeight": 1.0
}
```

**Response (Success):**
```json
{
  "status": "success",
  "calibrationFactor": 1.234567,
  "message": "Scale calibrated with known weight"
}
```

**Response (Error):**
```json
{
  "status": "error",
  "message": "No weight detected. Place weight on scale first."
}
```

#### POST `/api/tare`
Melakukan tare (reset ke nol).

**Response:**
```json
{
  "status": "success",
  "message": "Tare requested"
}
```

### 5. Reset Functions

#### POST `/api/reset`
Reset konfigurasi ke default.

**Response:**
```json
{
  "status": "success"
}
```

#### POST `/api/reset-defaults`
Reset semua konfigurasi ke factory defaults.

**Response:**
```json
{
  "status": "success",
  "message": "All configurations reset to defaults"
}
```

### 6. RFID User Management

#### GET `/api/rfid/users`
Mendapatkan daftar pengguna RFID.

**Response:**
```json
{
  "success": true,
  "users": [
    {
      "uid": "A1B2C3D4",
      "name": "John Doe",
      "authorized": true
    },
    {
      "uid": "12345678",
      "name": "Jane Smith",
      "authorized": true
    }
  ]
}
```

#### POST `/api/rfid/users`
Menambah pengguna RFID baru.

**Request Body:**
```json
{
  "uid": "A1B2C3D4",
  "name": "John Doe",
  "email": "john@example.com"
}
```

**Response (Success):**
```json
{
  "success": true,
  "message": "RFID user added successfully"
}
```

**Response (Error):**
```json
{
  "success": false,
  "message": "UID and name are required"
}
```

#### POST `/api/add-rfid-user`
Alias untuk `/api/rfid/users` POST endpoint.

#### POST `/api/rfid-sync`
Sinkronisasi data pengguna RFID dari Firebase.

**Response:**
```json
{
  "status": "success",
  "message": "RFID data synced",
  "cachedUsers": 5
}
```

### 7. Weight Data

#### GET `/weight`
Mendapatkan berat saat ini dalam format plain text.

**Response:**
```
1.234
```

---

## 📊 DATA MODELS

### WeightData Object
```json
{
  "raw": 1.235,           // Raw sensor reading
  "filtered": 1.234,      // Filtered reading
  "stable": 1.234,        // Last stable reading
  "isStable": true,       // Stability status
  "hasMotion": false,     // Motion detection
  "quality": "stable",    // Quality indicator
  "lastUpdate": 1234567890 // Timestamp
}
```

### User Object
```json
{
  "uid": "A1B2C3D4",      // RFID UID (8 hex chars)
  "name": "John Doe",     // User display name
  "email": "john@example.com", // Email (optional)
  "authorized": true,     // Authorization status
  "created_at": "1234567890" // Creation timestamp
}
```

### Configuration Object
```json
{
  "baseMode": true,       // Base weight correction enabled
  "baseWeight": 0.100,    // Base weight in kg
  "calibrationFactor": 1.0, // Calibration multiplier
  "stabilizationTime": 3, // Stability wait time (seconds)
  "weightThreshold": 1.0, // Change threshold (grams)
  "serverURL": "http://...", // Server URL
  "sessionTimeout": 5     // Session timeout (minutes)
}
```

---

## 🔧 ERROR HANDLING

### HTTP Status Codes
- **200**: Success
- **400**: Bad Request (invalid parameters)
- **404**: Not Found (endpoint doesn't exist)
- **500**: Internal Server Error

### Error Response Format
```json
{
  "status": "error",
  "message": "Descriptive error message",
  "code": "ERROR_CODE" // Optional error code
}
```

### Common Error Messages
- `"UID and name are required"` - Missing required fields
- `"Invalid UID format"` - UID not 8 hex characters
- `"No weight detected"` - No weight on scale for calibration
- `"Weight not stable"` - Weight fluctuating during calibration
- `"Firebase not ready"` - Firebase connection issue
- `"Network error"` - WiFi/Internet connection issue

---

## 📝 USAGE EXAMPLES

### JavaScript (Fetch API)

#### Get System Status
```javascript
fetch('http://192.168.1.100/status')
  .then(response => response.json())
  .then(data => {
    console.log('Weight:', data.weight);
    console.log('User:', data.authorizedUser);
  })
  .catch(error => console.error('Error:', error));
```

#### Add RFID User
```javascript
const userData = {
  uid: 'A1B2C3D4',
  name: 'John Doe',
  email: 'john@example.com'
};

fetch('http://192.168.1.100/api/rfid/users', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
  },
  body: JSON.stringify(userData)
})
.then(response => response.json())
.then(data => {
  if (data.success) {
    console.log('User added successfully');
  } else {
    console.error('Error:', data.message);
  }
});
```

#### Calibrate Scale
```javascript
const calibrationData = {
  knownWeight: 1.0
};

fetch('http://192.168.1.100/api/calibrate', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
  },
  body: JSON.stringify(calibrationData)
})
.then(response => response.json())
.then(data => {
  if (data.status === 'success') {
    console.log('Calibration successful');
    console.log('New factor:', data.calibrationFactor);
  }
});
```

### Python (requests)

#### Get System Status
```python
import requests

response = requests.get('http://192.168.1.100/status')
data = response.json()

print(f"Weight: {data['weight']} kg")
print(f"User: {data['authorizedUser']}")
print(f"Status: {data['access_status']}")
```

#### Add RFID User
```python
import requests

user_data = {
    'uid': 'A1B2C3D4',
    'name': 'John Doe',
    'email': 'john@example.com'
}

response = requests.post(
    'http://192.168.1.100/api/rfid/users',
    json=user_data
)

result = response.json()
if result['success']:
    print('User added successfully')
else:
    print(f"Error: {result['message']}")
```

### cURL

#### Get System Status
```bash
curl -X GET http://192.168.1.100/status
```

#### Add RFID User
```bash
curl -X POST http://192.168.1.100/api/rfid/users \
  -H "Content-Type: application/json" \
  -d '{
    "uid": "A1B2C3D4",
    "name": "John Doe",
    "email": "john@example.com"
  }'
```

#### Calibrate Scale
```bash
curl -X POST http://192.168.1.100/api/calibrate \
  -H "Content-Type: application/json" \
  -d '{"knownWeight": 1.0}'
```

---

## 🔒 SECURITY CONSIDERATIONS

### Authentication
- Currently no authentication required for local network access
- Firebase handles cloud data authentication
- Consider implementing API key authentication for production

### Input Validation
- UID must be exactly 8 hexadecimal characters
- Weight values must be positive numbers
- Name fields have length limits
- Email validation for proper format

### Rate Limiting
- No built-in rate limiting currently
- Consider implementing for production use
- Monitor for excessive requests

---

## 🚀 INTEGRATION EXAMPLES

### Real-time Dashboard
```javascript
// Update dashboard every 2 seconds
setInterval(() => {
  fetch('/status')
    .then(response => response.json())
    .then(data => {
      document.getElementById('weight').textContent = data.weight + ' kg';
      document.getElementById('user').textContent = data.authorizedUser;
      document.getElementById('status').textContent = data.access_status;
    });
}, 2000);
```

### Automated Data Collection
```python
import requests
import time
import csv

def collect_weight_data():
    while True:
        try:
            response = requests.get('http://192.168.1.100/status')
            data = response.json()
            
            # Save to CSV
            with open('weight_data.csv', 'a', newline='') as file:
                writer = csv.writer(file)
                writer.writerow([
                    time.time(),
                    data['weight'],
                    data['authorizedUser'],
                    data['isStable']
                ])
            
            time.sleep(5)  # Collect every 5 seconds
            
        except Exception as e:
            print(f"Error: {e}")
            time.sleep(10)

collect_weight_data()
```

### Mobile App Integration
```javascript
// React Native example
const WeightDisplay = () => {
  const [weightData, setWeightData] = useState(null);
  
  useEffect(() => {
    const interval = setInterval(async () => {
      try {
        const response = await fetch('http://192.168.1.100/status');
        const data = await response.json();
        setWeightData(data);
      } catch (error) {
        console.error('Failed to fetch weight data:', error);
      }
    }, 1000);
    
    return () => clearInterval(interval);
  }, []);
  
  return (
    <View>
      <Text>Weight: {weightData?.weight} kg</Text>
      <Text>User: {weightData?.authorizedUser}</Text>
      <Text>Status: {weightData?.access_status}</Text>
    </View>
  );
};
```

---

## 📚 ADDITIONAL RESOURCES

### Related Documentation
- [Installation Manual](INSTALLATION_MANUAL.md)
- [User Guide Manual](USER_GUIDE_MANUAL.md)
- [Code Documentation](PENJELASAN_KODE_UNTUK_SIDANG.md)

### External APIs
- **Firebase Realtime Database**: For cloud data storage
- **NTP Servers**: For time synchronization
- **WiFi Manager**: For network configuration

### Development Tools
- **Postman**: For API testing
- **curl**: Command-line HTTP client
- **Browser DevTools**: For web interface debugging

---

*API Documentation v1.2.0 - Last updated: [Current Date]*