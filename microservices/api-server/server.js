const express = require('express');
const cors = require('cors');
const axios = require('axios');
const WebSocket = require('ws');

const app = express();
const PORT = process.env.PORT || 3000;
const REGISTRY_URL = process.env.REGISTRY_URL || 'http://localhost:3001';

// Middleware
app.use(cors());
app.use(express.json());

// WebSocket server for real-time updates
const wss = new WebSocket.Server({ port: 3002 });

// In-memory storage (in production, use proper database)
let weightData = {
  raw: 0.0,
  filtered: 0.0,
  final: 0.0,
  isStable: false,
  isCalibrated: false,
  status: "standby",
  timestamp: new Date().toISOString()
};

let sessionData = {
  active: false,
  userUID: "",
  startTime: null,
  lastActivity: null
};

let configData = {
  baseMode: false,
  baseWeight: 0.0,
  scaleFactor: 1.0,
  stabilizationTime: 3000
};

// Broadcast to all WebSocket clients
function broadcast(data) {
  wss.clients.forEach(client => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(data));
    }
  });
}

// WebSocket connection handler
wss.on('connection', (ws) => {
  console.log('[WEBSOCKET] Client connected');
  
  // Send current data to new client
  ws.send(JSON.stringify({
    type: 'initial_data',
    weightData: weightData,
    sessionData: sessionData,
    configData: configData
  }));
  
  ws.on('close', () => {
    console.log('[WEBSOCKET] Client disconnected');
  });
});

// Routes
app.get('/health', (req, res) => {
  res.json({ 
    status: 'healthy', 
    service: 'api-server',
    timestamp: new Date().toISOString(),
    connections: wss.clients.size
  });
});

// Weight data endpoints
app.get('/api/status', (req, res) => {
  res.json({
    weightData: weightData,
    sessionData: sessionData,
    configData: configData,
    timestamp: new Date().toISOString()
  });
});

app.post('/api/weight-data', (req, res) => {
  const { raw, filtered, final, isStable, isCalibrated, status } = req.body;
  
  // Update weight data
  weightData = {
    raw: parseFloat(raw) || 0.0,
    filtered: parseFloat(filtered) || 0.0,
    final: parseFloat(final) || 0.0,
    isStable: Boolean(isStable),
    isCalibrated: Boolean(isCalibrated),
    status: status || "unknown",
    timestamp: new Date().toISOString()
  };
  
  // Broadcast to WebSocket clients
  broadcast({
    type: 'weight_update',
    data: weightData
  });
  
  console.log(`[WEIGHT] Raw: ${weightData.raw}kg, Final: ${weightData.final}kg, Stable: ${weightData.isStable}`);
  
  res.json({
    status: 'success',
    message: 'Weight data updated',
    data: weightData
  });
});

// Configuration endpoints
app.get('/api/config', (req, res) => {
  res.json({
    config: configData,
    timestamp: new Date().toISOString()
  });
});

app.post('/api/config', (req, res) => {
  const { baseMode, baseWeight, scaleFactor, stabilizationTime } = req.body;
  
  if (baseMode !== undefined) configData.baseMode = Boolean(baseMode);
  if (baseWeight !== undefined) configData.baseWeight = parseFloat(baseWeight);
  if (scaleFactor !== undefined) configData.scaleFactor = parseFloat(scaleFactor);
  if (stabilizationTime !== undefined) configData.stabilizationTime = parseInt(stabilizationTime);
  
  // Broadcast config update
  broadcast({
    type: 'config_update',
    data: configData
  });
  
  console.log('[CONFIG] Configuration updated:', configData);
  
  res.json({
    status: 'success',
    message: 'Configuration updated',
    config: configData
  });
});

// Calibration endpoints
app.post('/api/calibrate', (req, res) => {
  const { weight } = req.body;
  
  if (!weight || weight <= 0) {
    return res.status(400).json({
      status: 'error',
      message: 'Valid weight value required for calibration'
    });
  }
  
  if (!weightData.isStable) {
    return res.status(400).json({
      status: 'error',
      message: 'Weight not stable. Wait for stable reading before calibration.'
    });
  }
  
  // Perform calibration
  configData.baseWeight = parseFloat(weight);
  configData.baseMode = true;
  weightData.isCalibrated = true;
  
  // Broadcast calibration update
  broadcast({
    type: 'calibration_update',
    data: {
      baseWeight: configData.baseWeight,
      isCalibrated: weightData.isCalibrated
    }
  });
  
  console.log(`[CALIBRATION] Base weight set to: ${configData.baseWeight}kg`);
  
  res.json({
    status: 'success',
    message: 'Calibration completed successfully',
    baseWeight: configData.baseWeight
  });
});

app.post('/api/reset', (req, res) => {
  // Reset configuration to defaults
  configData = {
    baseMode: false,
    baseWeight: 0.0,
    scaleFactor: 1.0,
    stabilizationTime: 3000
  };
  
  weightData.isCalibrated = false;
  
  // Broadcast reset
  broadcast({
    type: 'system_reset',
    data: { configData, weightData }
  });
  
  console.log('[RESET] System configuration reset to defaults');
  
  res.json({
    status: 'success',
    message: 'System reset successfully'
  });
});

app.post('/api/tare', (req, res) => {
  // Tare functionality - zero the scale
  const tareValue = weightData.raw;
  
  // Broadcast tare command
  broadcast({
    type: 'tare_command',
    data: { tareValue: tareValue }
  });
  
  console.log(`[TARE] Tare command issued with value: ${tareValue}kg`);
  
  res.json({
    status: 'success',
    message: 'Tare command issued',
    tareValue: tareValue
  });
});

// Session management endpoints
app.get('/api/session', (req, res) => {
  res.json({
    session: sessionData,
    timestamp: new Date().toISOString()
  });
});

app.post('/api/session/start', (req, res) => {
  const { userUID } = req.body;
  
  if (!userUID) {
    return res.status(400).json({
      status: 'error',
      message: 'User UID required to start session'
    });
  }
  
  sessionData = {
    active: true,
    userUID: userUID,
    startTime: new Date().toISOString(),
    lastActivity: new Date().toISOString()
  };
  
  // Broadcast session start
  broadcast({
    type: 'session_start',
    data: sessionData
  });
  
  console.log(`[SESSION] Started for user: ${userUID}`);
  
  res.json({
    status: 'success',
    message: 'Session started successfully',
    session: sessionData
  });
});

app.post('/api/session/end', (req, res) => {
  const previousSession = { ...sessionData };
  
  sessionData = {
    active: false,
    userUID: "",
    startTime: null,
    lastActivity: null
  };
  
  // Broadcast session end
  broadcast({
    type: 'session_end',
    data: { previousSession, currentSession: sessionData }
  });
  
  console.log(`[SESSION] Ended for user: ${previousSession.userUID}`);
  
  res.json({
    status: 'success',
    message: 'Session ended successfully',
    previousSession: previousSession
  });
});

// Register with service registry
async function registerWithRegistry() {
  try {
    const response = await axios.post(`${REGISTRY_URL}/services/api-server`, {
      url: `http://localhost:${PORT}`,
      endpoints: [
        '/api/status',
        '/api/config',
        '/api/calibrate',
        '/api/reset',
        '/api/tare',
        '/api/weight-data',
        '/api/session'
      ]
    });
    console.log('[REGISTRY] Successfully registered with service registry');
  } catch (error) {
    console.error('[REGISTRY] Failed to register with service registry:', error.message);
  }
}

// Start server
app.listen(PORT, () => {
  console.log(`[API SERVER] Running on port ${PORT}`);
  console.log(`[WEBSOCKET] WebSocket server running on port 3002`);
  
  // Register with service registry after a delay
  setTimeout(registerWithRegistry, 2000);
});
