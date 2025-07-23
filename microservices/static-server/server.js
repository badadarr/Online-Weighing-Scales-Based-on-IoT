const express = require('express');
const cors = require('cors');
const axios = require('axios');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 8080;
const REGISTRY_URL = process.env.REGISTRY_URL || 'http://localhost:3001';
const API_SERVER_URL = process.env.API_SERVER_URL || 'http://localhost:3000';

// Middleware
app.use(cors());
app.use(express.json());

// Serve static files from public directory
app.use('/assets', express.static(path.join(__dirname, 'public')));

// Health check
app.get('/health', (req, res) => {
  res.json({ 
    status: 'healthy', 
    service: 'static-server',
    timestamp: new Date().toISOString()
  });
});

// Main dashboard page
app.get('/', (req, res) => {
  res.send(getMainPageHTML());
});

// Configuration page
app.get('/config', (req, res) => {
  res.send(getConfigPageHTML());
});

// Dashboard page with real-time data
app.get('/dashboard', (req, res) => {
  res.send(getDashboardPageHTML());
});

// Main page HTML
function getMainPageHTML() {
  return `
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IoT Weighing Scale System</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
        }
        
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.1);
            padding: 40px;
            text-align: center;
            max-width: 500px;
            width: 90%;
        }
        
        .logo {
            width: 80px;
            height: 80px;
            background: linear-gradient(45deg, #667eea, #764ba2);
            border-radius: 50%;
            margin: 0 auto 20px;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-size: 24px;
            font-weight: bold;
        }
        
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 28px;
        }
        
        .subtitle {
            color: #666;
            margin-bottom: 30px;
            font-size: 16px;
        }
        
        .nav-buttons {
            display: flex;
            flex-direction: column;
            gap: 15px;
            margin-top: 30px;
        }
        
        .nav-button {
            background: linear-gradient(45deg, #667eea, #764ba2);
            color: white;
            border: none;
            padding: 15px 30px;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            text-decoration: none;
            display: inline-block;
        }
        
        .nav-button:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 30px rgba(102, 126, 234, 0.4);
        }
        
        .nav-button.secondary {
            background: linear-gradient(45deg, #f093fb, #f5576c);
        }
        
        .nav-button.secondary:hover {
            box-shadow: 0 10px 30px rgba(245, 87, 108, 0.4);
        }
        
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
        }
        
        .status-online {
            background: #4CAF50;
            box-shadow: 0 0 10px rgba(76, 175, 80, 0.5);
        }
        
        .status-offline {
            background: #f44336;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">⚖️</div>
        <h1>IoT Weighing Scale</h1>
        <p class="subtitle">Sistem Timbangan Digital Berbasis Internet of Things</p>
        
        <div id="systemStatus">
            <p><span class="status-indicator status-offline" id="statusIndicator"></span>
            <span id="statusText">Connecting to system...</span></p>
        </div>
        
        <div class="nav-buttons">
            <a href="/dashboard" class="nav-button">
                📊 Dashboard Real-time
            </a>
            <a href="/config" class="nav-button secondary">
                ⚙️ Konfigurasi Sistem
            </a>
        </div>
    </div>

    <script>
        // Check system status
        async function checkSystemStatus() {
            try {
                const response = await fetch('${API_SERVER_URL}/api/status');
                if (response.ok) {
                    document.getElementById('statusIndicator').className = 'status-indicator status-online';
                    document.getElementById('statusText').textContent = 'System Online';
                } else {
                    throw new Error('API not responding');
                }
            } catch (error) {
                document.getElementById('statusIndicator').className = 'status-indicator status-offline';
                document.getElementById('statusText').textContent = 'System Offline';
            }
        }
        
        // Check status on load and every 10 seconds
        checkSystemStatus();
        setInterval(checkSystemStatus, 10000);
    </script>
</body>
</html>
  `;
}

// Configuration page HTML
function getConfigPageHTML() {
  return `
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Konfigurasi - IoT Weighing Scale</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.1);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(45deg, #667eea, #764ba2);
            color: white;
            padding: 30px;
            text-align: center;
        }
        
        .content {
            padding: 30px;
        }
        
        .config-section {
            margin-bottom: 30px;
            padding: 20px;
            border: 1px solid #e0e0e0;
            border-radius: 10px;
        }
        
        .section-title {
            font-size: 18px;
            font-weight: 600;
            margin-bottom: 15px;
            color: #333;
        }
        
        .form-group {
            margin-bottom: 15px;
        }
        
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: 500;
            color: #555;
        }
        
        input, select {
            width: 100%;
            padding: 10px;
            border: 2px solid #e0e0e0;
            border-radius: 5px;
            font-size: 14px;
        }
        
        input:focus, select:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .btn {
            background: linear-gradient(45deg, #667eea, #764ba2);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 5px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            margin-right: 10px;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        
        .btn-danger {
            background: linear-gradient(45deg, #f093fb, #f5576c);
        }
        
        .btn-success {
            background: linear-gradient(45deg, #4CAF50, #45a049);
        }
        
        .status-display {
            background: #f5f5f5;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
        }
        
        .weight-display {
            font-size: 24px;
            font-weight: bold;
            text-align: center;
            margin: 10px 0;
        }
        
        .back-link {
            display: inline-block;
            margin-top: 20px;
            color: #667eea;
            text-decoration: none;
            font-weight: 500;
        }
        
        .back-link:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>⚙️ Konfigurasi Sistem</h1>
            <p>Pengaturan dan kalibrasi timbangan IoT</p>
        </div>
        
        <div class="content">
            <div class="status-display">
                <h3>Status Sistem</h3>
                <div class="weight-display" id="currentWeight">0.000 kg</div>
                <div id="systemStatus">Status: <span id="statusText">Loading...</span></div>
                <div>Koneksi: <span id="connectionStatus">Checking...</span></div>
            </div>
            
            <div class="config-section">
                <h2 class="section-title">Konfigurasi Base Weight</h2>
                <div class="form-group">
                    <label>
                        <input type="checkbox" id="baseMode"> Aktifkan Mode Base Weight
                    </label>
                </div>
                <div class="form-group">
                    <label for="baseWeight">Base Weight (kg):</label>
                    <input type="number" id="baseWeight" step="0.001" placeholder="0.000">
                </div>
                <button class="btn btn-success" onclick="calibrateBase()">Kalibrasi Base</button>
                <button class="btn" onclick="saveConfig()">Simpan Konfigurasi</button>
            </div>
            
            <div class="config-section">
                <h2 class="section-title">Kontrol Timbangan</h2>
                <button class="btn" onclick="tareScale()">Tare (Zero)</button>
                <button class="btn btn-danger" onclick="resetSystem()">Reset Sistem</button>
            </div>
            
            <a href="/" class="back-link">← Kembali ke Dashboard</a>
        </div>
    </div>

    <script>
        const API_BASE = '${API_SERVER_URL}/api';
        
        // Load current configuration
        async function loadConfig() {
            try {
                const response = await fetch(\`\${API_BASE}/config\`);
                const data = await response.json();
                
                document.getElementById('baseMode').checked = data.config.baseMode;
                document.getElementById('baseWeight').value = data.config.baseWeight;
            } catch (error) {
                console.error('Error loading config:', error);
            }
        }
        
        // Update real-time status
        async function updateStatus() {
            try {
                const response = await fetch(\`\${API_BASE}/status\`);
                const data = await response.json();
                
                document.getElementById('currentWeight').textContent = \`\${data.weightData.final.toFixed(3)} kg\`;
                document.getElementById('statusText').textContent = data.weightData.status;
                document.getElementById('connectionStatus').textContent = 'Online';
                document.getElementById('connectionStatus').style.color = '#4CAF50';
            } catch (error) {
                document.getElementById('connectionStatus').textContent = 'Offline';
                document.getElementById('connectionStatus').style.color = '#f44336';
            }
        }
        
        // Save configuration
        async function saveConfig() {
            const config = {
                baseMode: document.getElementById('baseMode').checked,
                baseWeight: parseFloat(document.getElementById('baseWeight').value) || 0
            };
            
            try {
                const response = await fetch(\`\${API_BASE}/config\`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config)
                });
                
                if (response.ok) {
                    alert('Konfigurasi berhasil disimpan!');
                } else {
                    alert('Gagal menyimpan konfigurasi');
                }
            } catch (error) {
                alert('Error: ' + error.message);
            }
        }
        
        // Calibrate base weight
        async function calibrateBase() {
            if (!confirm('Pastikan hanya base/wadah yang ada di timbangan. Lanjutkan kalibrasi?')) {
                return;
            }
            
            try {
                const response = await fetch(\`\${API_BASE}/calibrate\`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ weight: parseFloat(document.getElementById('baseWeight').value) || 0 })
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    alert('Kalibrasi berhasil!');
                    loadConfig();
                } else {
                    alert('Kalibrasi gagal: ' + result.message);
                }
            } catch (error) {
                alert('Error: ' + error.message);
            }
        }
        
        // Tare scale
        async function tareScale() {
            try {
                const response = await fetch(\`\${API_BASE}/tare\`, {
                    method: 'POST'
                });
                
                if (response.ok) {
                    alert('Tare berhasil dilakukan!');
                } else {
                    alert('Gagal melakukan tare');
                }
            } catch (error) {
                alert('Error: ' + error.message);
            }
        }
        
        // Reset system
        async function resetSystem() {
            if (!confirm('Reset akan menghapus semua konfigurasi. Lanjutkan?')) {
                return;
            }
            
            try {
                const response = await fetch(\`\${API_BASE}/reset\`, {
                    method: 'POST'
                });
                
                if (response.ok) {
                    alert('Sistem berhasil direset!');
                    loadConfig();
                } else {
                    alert('Gagal reset sistem');
                }
            } catch (error) {
                alert('Error: ' + error.message);
            }
        }
        
        // Initialize
        loadConfig();
        updateStatus();
        setInterval(updateStatus, 2000);
    </script>
</body>
</html>
  `;
}

// Dashboard page HTML
function getDashboardPageHTML() {
  return `
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dashboard - IoT Weighing Scale</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: #f5f7fa;
            min-height: 100vh;
        }
        
        .header {
            background: linear-gradient(45deg, #667eea, #764ba2);
            color: white;
            padding: 20px 0;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        
        .header-content {
            max-width: 1200px;
            margin: 0 auto;
            padding: 0 20px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
        }
        
        .dashboard-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .card {
            background: white;
            border-radius: 15px;
            padding: 25px;
            box-shadow: 0 5px 20px rgba(0,0,0,0.1);
            transition: transform 0.3s ease;
        }
        
        .card:hover {
            transform: translateY(-5px);
        }
        
        .card-title {
            font-size: 18px;
            font-weight: 600;
            margin-bottom: 15px;
            color: #333;
        }
        
        .weight-display {
            font-size: 48px;
            font-weight: bold;
            text-align: center;
            margin: 20px 0;
            color: #667eea;
        }
        
        .status-badge {
            display: inline-block;
            padding: 5px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 600;
            text-transform: uppercase;
        }
        
        .status-stable {
            background: #e8f5e8;
            color: #4CAF50;
        }
        
        .status-unstable {
            background: #fff3e0;
            color: #ff9800;
        }
        
        .status-active {
            background: #e3f2fd;
            color: #2196f3;
        }
        
        .status-inactive {
            background: #fce4ec;
            color: #e91e63;
        }
        
        .connection-indicator {
            width: 12px;
            height: 12px;
            border-radius: 50%;
            display: inline-block;
            margin-right: 8px;
        }
        
        .connected {
            background: #4CAF50;
            box-shadow: 0 0 10px rgba(76, 175, 80, 0.5);
        }
        
        .disconnected {
            background: #f44336;
        }
        
        .data-table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 15px;
        }
        
        .data-table th,
        .data-table td {
            padding: 10px;
            text-align: left;
            border-bottom: 1px solid #e0e0e0;
        }
        
        .data-table th {
            background: #f8f9fa;
            font-weight: 600;
        }
        
        .nav-link {
            color: white;
            text-decoration: none;
            margin-left: 20px;
            padding: 8px 15px;
            border-radius: 5px;
            transition: background 0.3s ease;
        }
        
        .nav-link:hover {
            background: rgba(255,255,255,0.2);
        }
    </style>
</head>
<body>
    <div class="header">
        <div class="header-content">
            <h1>📊 Dashboard Real-time</h1>
            <nav>
                <a href="/" class="nav-link">Home</a>
                <a href="/config" class="nav-link">Konfigurasi</a>
            </nav>
        </div>
    </div>
    
    <div class="container">
        <div class="dashboard-grid">
            <div class="card">
                <h2 class="card-title">Berat Saat Ini</h2>
                <div class="weight-display" id="currentWeight">0.000 kg</div>
                <div>
                    Status: <span class="status-badge status-unstable" id="weightStatus">Loading...</span>
                </div>
            </div>
            
            <div class="card">
                <h2 class="card-title">Status Koneksi</h2>
                <div style="margin: 20px 0;">
                    <div>
                        <span class="connection-indicator disconnected" id="wsIndicator"></span>
                        WebSocket: <span id="wsStatus">Connecting...</span>
                    </div>
                    <div style="margin-top: 10px;">
                        <span class="connection-indicator disconnected" id="apiIndicator"></span>
                        API Server: <span id="apiStatus">Checking...</span>
                    </div>
                </div>
            </div>
            
            <div class="card">
                <h2 class="card-title">Informasi Sesi</h2>
                <div>
                    Status: <span class="status-badge status-inactive" id="sessionStatus">Tidak Aktif</span>
                </div>
                <div style="margin-top: 10px;">
                    User: <span id="sessionUser">-</span>
                </div>
                <div style="margin-top: 5px;">
                    Mulai: <span id="sessionStart">-</span>
                </div>
            </div>
        </div>
        
        <div class="card">
            <h2 class="card-title">Data Detail</h2>
            <table class="data-table">
                <thead>
                    <tr>
                        <th>Parameter</th>
                        <th>Nilai</th>
                        <th>Status</th>
                    </tr>
                </thead>
                <tbody>
                    <tr>
                        <td>Raw Weight</td>
                        <td><span id="rawWeight">0.000 kg</span></td>
                        <td><span id="rawStatus">-</span></td>
                    </tr>
                    <tr>
                        <td>Filtered Weight</td>
                        <td><span id="filteredWeight">0.000 kg</span></td>
                        <td><span id="filteredStatus">-</span></td>
                    </tr>
                    <tr>
                        <td>Final Weight</td>
                        <td><span id="finalWeight">0.000 kg</span></td>
                        <td><span id="finalStatus">-</span></td>
                    </tr>
                    <tr>
                        <td>Kalibrasi</td>
                        <td><span id="calibrationStatus">-</span></td>
                        <td><span id="calibrationBadge">-</span></td>
                    </tr>
                </tbody>
            </table>
        </div>
    </div>

    <script>
        const WS_URL = 'ws://localhost:3002';
        const API_BASE = '${API_SERVER_URL}/api';
        
        let ws = null;
        let reconnectTimer = null;
        
        // WebSocket connection
        function connectWebSocket() {
            try {
                ws = new WebSocket(WS_URL);
                
                ws.onopen = function() {
                    console.log('WebSocket connected');
                    document.getElementById('wsIndicator').className = 'connection-indicator connected';
                    document.getElementById('wsStatus').textContent = 'Connected';
                    document.getElementById('wsStatus').style.color = '#4CAF50';
                };
                
                ws.onmessage = function(event) {
                    const data = JSON.parse(event.data);
                    handleWebSocketMessage(data);
                };
                
                ws.onclose = function() {
                    console.log('WebSocket disconnected');
                    document.getElementById('wsIndicator').className = 'connection-indicator disconnected';
                    document.getElementById('wsStatus').textContent = 'Disconnected';
                    document.getElementById('wsStatus').style.color = '#f44336';
                    
                    // Reconnect after 3 seconds
                    clearTimeout(reconnectTimer);
                    reconnectTimer = setTimeout(connectWebSocket, 3000);
                };
                
                ws.onerror = function(error) {
                    console.error('WebSocket error:', error);
                };
                
            } catch (error) {
                console.error('WebSocket connection error:', error);
                clearTimeout(reconnectTimer);
                reconnectTimer = setTimeout(connectWebSocket, 3000);
            }
        }
        
        // Handle WebSocket messages
        function handleWebSocketMessage(data) {
            switch(data.type) {
                case 'initial_data':
                case 'weight_update':
                    updateWeightDisplay(data.weightData || data.data);
                    break;
                case 'session_start':
                case 'session_end':
                    updateSessionDisplay(data.data);
                    break;
                case 'config_update':
                    console.log('Config updated:', data.data);
                    break;
            }
        }
        
        // Update weight display
        function updateWeightDisplay(weightData) {
            document.getElementById('currentWeight').textContent = \`\${weightData.final.toFixed(3)} kg\`;
            document.getElementById('rawWeight').textContent = \`\${weightData.raw.toFixed(3)} kg\`;
            document.getElementById('filteredWeight').textContent = \`\${weightData.filtered.toFixed(3)} kg\`;
            document.getElementById('finalWeight').textContent = \`\${weightData.final.toFixed(3)} kg\`;
            
            // Update status badges
            const statusElement = document.getElementById('weightStatus');
            if (weightData.isStable) {
                statusElement.className = 'status-badge status-stable';
                statusElement.textContent = 'Stabil';
            } else {
                statusElement.className = 'status-badge status-unstable';
                statusElement.textContent = 'Tidak Stabil';
            }
            
            // Update calibration status
            document.getElementById('calibrationStatus').textContent = weightData.isCalibrated ? 'Terkalibrasi' : 'Belum Kalibrasi';
            const calibBadge = document.getElementById('calibrationBadge');
            if (weightData.isCalibrated) {
                calibBadge.className = 'status-badge status-stable';
                calibBadge.textContent = 'OK';
            } else {
                calibBadge.className = 'status-badge status-unstable';
                calibBadge.textContent = 'Perlu Kalibrasi';
            }
        }
        
        // Update session display
        function updateSessionDisplay(sessionData) {
            const statusElement = document.getElementById('sessionStatus');
            if (sessionData.active) {
                statusElement.className = 'status-badge status-active';
                statusElement.textContent = 'Aktif';
                document.getElementById('sessionUser').textContent = sessionData.userUID;
                document.getElementById('sessionStart').textContent = new Date(sessionData.startTime).toLocaleString('id-ID');
            } else {
                statusElement.className = 'status-badge status-inactive';
                statusElement.textContent = 'Tidak Aktif';
                document.getElementById('sessionUser').textContent = '-';
                document.getElementById('sessionStart').textContent = '-';
            }
        }
        
        // Check API status
        async function checkAPIStatus() {
            try {
                const response = await fetch(\`\${API_BASE}/status\`);
                if (response.ok) {
                    document.getElementById('apiIndicator').className = 'connection-indicator connected';
                    document.getElementById('apiStatus').textContent = 'Online';
                    document.getElementById('apiStatus').style.color = '#4CAF50';
                    
                    const data = await response.json();
                    updateWeightDisplay(data.weightData);
                    updateSessionDisplay(data.sessionData);
                } else {
                    throw new Error('API not responding');
                }
            } catch (error) {
                document.getElementById('apiIndicator').className = 'connection-indicator disconnected';
                document.getElementById('apiStatus').textContent = 'Offline';
                document.getElementById('apiStatus').style.color = '#f44336';
            }
        }
        
        // Initialize
        connectWebSocket();
        checkAPIStatus();
        setInterval(checkAPIStatus, 10000);
    </script>
</body>
</html>
  `;
}

// Register with service registry
async function registerWithRegistry() {
  try {
    const response = await axios.post(`${REGISTRY_URL}/services/static-server`, {
      url: `http://localhost:${PORT}`,
      endpoints: [
        '/',
        '/config',
        '/dashboard',
        '/assets/*'
      ]
    });
    console.log('[REGISTRY] Successfully registered with service registry');
  } catch (error) {
    console.error('[REGISTRY] Failed to register with service registry:', error.message);
  }
}

// Start server
app.listen(PORT, () => {
  console.log(`[STATIC SERVER] Running on port ${PORT}`);
  
  // Register with service registry after a delay
  setTimeout(registerWithRegistry, 2000);
});
