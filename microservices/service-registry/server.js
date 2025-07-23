const express = require('express');
const cors = require('cors');
const axios = require('axios');

const app = express();
const PORT = process.env.PORT || 3001;

// Middleware
app.use(cors());
app.use(express.json());

// Service registry storage
const services = {
  'api-server': {
    name: 'api-server',
    url: 'http://localhost:3000',
    status: 'unknown',
    lastCheck: null,
    endpoints: [
      '/api/status',
      '/api/config',
      '/api/calibrate',
      '/api/reset',
      '/api/tare',
      '/api/weight-data',
      '/api/session'
    ]
  },
  'static-server': {
    name: 'static-server',
    url: 'http://localhost:8080',
    status: 'unknown',
    lastCheck: null,
    endpoints: [
      '/',
      '/config',
      '/dashboard',
      '/assets/*'
    ]
  }
};

// Health check function
async function checkServiceHealth(serviceName, serviceData) {
  try {
    const response = await axios.get(`${serviceData.url}/health`, {
      timeout: 5000
    });
    
    services[serviceName].status = response.status === 200 ? 'healthy' : 'unhealthy';
    services[serviceName].lastCheck = new Date().toISOString();
    
    console.log(`[HEALTH CHECK] ${serviceName}: ${services[serviceName].status}`);
  } catch (error) {
    services[serviceName].status = 'unhealthy';
    services[serviceName].lastCheck = new Date().toISOString();
    console.log(`[HEALTH CHECK] ${serviceName}: unhealthy - ${error.message}`);
  }
}

// Routes
app.get('/health', (req, res) => {
  res.json({ 
    status: 'healthy', 
    service: 'service-registry',
    timestamp: new Date().toISOString()
  });
});

// Get all services
app.get('/services', (req, res) => {
  res.json({
    services: services,
    registry: {
      status: 'healthy',
      timestamp: new Date().toISOString()
    }
  });
});

// Get specific service
app.get('/services/:serviceName', (req, res) => {
  const serviceName = req.params.serviceName;
  
  if (services[serviceName]) {
    res.json({
      service: services[serviceName],
      timestamp: new Date().toISOString()
    });
  } else {
    res.status(404).json({
      error: 'Service not found',
      availableServices: Object.keys(services)
    });
  }
});

// Register new service
app.post('/services/:serviceName', (req, res) => {
  const serviceName = req.params.serviceName;
  const { url, endpoints } = req.body;
  
  if (!url) {
    return res.status(400).json({ error: 'Service URL is required' });
  }
  
  services[serviceName] = {
    name: serviceName,
    url: url,
    status: 'unknown',
    lastCheck: null,
    endpoints: endpoints || [],
    registeredAt: new Date().toISOString()
  };
  
  console.log(`[REGISTRY] Registered service: ${serviceName} at ${url}`);
  
  res.json({
    message: `Service ${serviceName} registered successfully`,
    service: services[serviceName]
  });
});

// Unregister service
app.delete('/services/:serviceName', (req, res) => {
  const serviceName = req.params.serviceName;
  
  if (services[serviceName]) {
    delete services[serviceName];
    console.log(`[REGISTRY] Unregistered service: ${serviceName}`);
    res.json({ message: `Service ${serviceName} unregistered successfully` });
  } else {
    res.status(404).json({ error: 'Service not found' });
  }
});

// Service discovery endpoint
app.get('/discover/:serviceName', (req, res) => {
  const serviceName = req.params.serviceName;
  
  if (services[serviceName] && services[serviceName].status === 'healthy') {
    res.json({
      service: services[serviceName],
      discovered: true
    });
  } else {
    res.status(503).json({
      error: 'Service unavailable',
      serviceName: serviceName,
      status: services[serviceName]?.status || 'not found'
    });
  }
});

// Periodic health checks
setInterval(() => {
  console.log('[REGISTRY] Running health checks...');
  Object.entries(services).forEach(([serviceName, serviceData]) => {
    checkServiceHealth(serviceName, serviceData);
  });
}, 30000); // Check every 30 seconds

// Initial health check
setTimeout(() => {
  console.log('[REGISTRY] Running initial health checks...');
  Object.entries(services).forEach(([serviceName, serviceData]) => {
    checkServiceHealth(serviceName, serviceData);
  });
}, 5000); // Wait 5 seconds for other services to start

app.listen(PORT, () => {
  console.log(`[SERVICE REGISTRY] Running on port ${PORT}`);
  console.log(`[SERVICE REGISTRY] Registered services: ${Object.keys(services).join(', ')}`);
});
