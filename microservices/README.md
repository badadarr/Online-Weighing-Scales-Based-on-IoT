# IoT Weighing Scale - Microservices Architecture

## Overview

Implementasi Microservices Architecture untuk sistem IoT Weighing Scale yang memisahkan tanggung jawab antara API handling, static file serving, dan service discovery.

## Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│  Static Server  │    │   API Server    │    │Service Registry │
│    Port 8080    │    │    Port 3000    │    │    Port 3001    │
│                 │    │                 │    │                 │
│ - HTML/CSS/JS   │    │ - IoT Data API  │    │ - Service Disc. │
│ - Web Interface │    │ - WebSocket     │    │ - Health Checks │
│ - Dashboard     │    │ - Session Mgmt  │    │ - Load Balancing│
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                    ┌─────────────────┐
                    │ Nginx Load Bal. │
                    │    Port 80      │
                    │                 │
                    │ - Route Traffic │
                    │ - SSL Termina.  │
                    │ - Rate Limiting │
                    └─────────────────┘
                                 │
                    ┌─────────────────┐
                    │   Client/IoT    │
                    │    Devices      │
                    └─────────────────┘
```

## Services

### 1. Service Registry (Port 3001)
- **Purpose**: Service discovery dan health monitoring
- **Features**:
  - Auto-registration services
  - Health checks setiap 30 detik
  - Service discovery endpoints
  - Centralized service management

### 2. API Server (Port 3000 + 3002)
- **Purpose**: Backend logic dan IoT data handling
- **Features**:
  - RESTful API untuk weight data
  - WebSocket real-time communication
  - Session management
  - Configuration management
  - Calibration endpoints

### 3. Static Server (Port 8080)
- **Purpose**: Frontend static files
- **Features**:
  - HTML/CSS/JS serving
  - Dashboard interface
  - Configuration pages
  - Asset management

### 4. Nginx Load Balancer (Port 80)
- **Purpose**: Traffic routing dan load balancing
- **Features**:
  - Route `/api/*` to API Server
  - Route `/*` to Static Server
  - WebSocket proxy support
  - Rate limiting
  - SSL termination support

## Quick Start

### Using Docker (Recommended)

1. **Start all services:**
   ```bash
   # Windows
   setup.bat
   
   # Linux/Mac
   chmod +x setup.sh
   ./setup.sh
   ```

2. **Access the application:**
   - Main App: http://localhost
   - Dashboard: http://localhost/dashboard
   - Config: http://localhost/config
   - API: http://localhost/api/status

### Manual Setup

1. **Install dependencies:**
   ```bash
   cd service-registry && npm install && cd ..
   cd api-server && npm install && cd ..
   cd static-server && npm install && cd ..
   ```

2. **Start services individually:**
   ```bash
   # Terminal 1 - Service Registry
   cd service-registry && npm start
   
   # Terminal 2 - API Server
   cd api-server && npm start
   
   # Terminal 3 - Static Server
   cd static-server && npm start
   ```

3. **Start Nginx (Optional):**
   ```bash
   docker run -p 80:80 -v $(pwd)/nginx/nginx.conf:/etc/nginx/nginx.conf nginx:alpine
   ```

## API Endpoints

### Weight Data
- `GET /api/status` - Get current system status
- `POST /api/weight-data` - Update weight data from IoT device

### Configuration
- `GET /api/config` - Get current configuration
- `POST /api/config` - Update configuration

### Calibration
- `POST /api/calibrate` - Perform calibration
- `POST /api/reset` - Reset system to defaults
- `POST /api/tare` - Tare/zero the scale

### Session Management
- `GET /api/session` - Get session status
- `POST /api/session/start` - Start user session
- `POST /api/session/end` - End user session

## WebSocket Events

Connect to `ws://localhost:3002` untuk real-time updates:

- `weight_update` - Weight data changes
- `session_start` - User session started
- `session_end` - User session ended
- `config_update` - Configuration changed
- `calibration_update` - Calibration performed

## Environment Variables

### API Server
- `PORT` - Server port (default: 3000)
- `REGISTRY_URL` - Service registry URL
- `NODE_ENV` - Environment (development/production)

### Static Server
- `PORT` - Server port (default: 8080)
- `API_SERVER_URL` - API server URL for frontend
- `REGISTRY_URL` - Service registry URL

### Service Registry
- `PORT` - Server port (default: 3001)

## ESP32 Integration

Untuk mengintegrasikan dengan ESP32, update endpoint di WebServer.cpp:

```cpp
// Ganti dari localhost ke load balancer
const char* API_ENDPOINT = "http://your-server/api/weight-data";
const char* WS_ENDPOINT = "ws://your-server/ws";
```

## Monitoring

### Health Checks
- Service Registry: http://localhost:3001/health
- API Server: http://localhost:3000/health
- Static Server: http://localhost:8080/health
- Load Balancer: http://localhost/health

### Service Discovery
- All services: http://localhost:3001/services
- Specific service: http://localhost:3001/services/api-server

### Logs
```bash
# View all logs
docker-compose logs -f

# View specific service
docker-compose logs -f api-server
```

## Production Deployment

1. **Environment Setup:**
   ```bash
   export NODE_ENV=production
   export REGISTRY_URL=http://your-registry-server:3001
   ```

2. **SSL Configuration:**
   - Update nginx.conf untuk HTTPS
   - Add SSL certificates di nginx/ssl/
   - Update client URLs ke https://

3. **Database Integration:**
   - Add Redis/PostgreSQL untuk persistent storage
   - Update API server untuk database connection
   - Implement data persistence

4. **Scaling:**
   ```bash
   # Scale API servers
   docker-compose up --scale api-server=3
   
   # Scale static servers
   docker-compose up --scale static-server=2
   ```

## Security Features

- Rate limiting (API: 10req/s, Static: 50req/s)
- CORS headers
- Security headers (XSS, CSRF protection)
- Health check authentication (optional)
- SSL/TLS termination support

## Troubleshooting

### Common Issues

1. **Services not connecting:**
   - Check Docker network connectivity
   - Verify service registry is running first
   - Check firewall/port access

2. **WebSocket connection failed:**
   - Ensure port 3002 is accessible
   - Check nginx WebSocket proxy config
   - Verify client WebSocket URL

3. **API requests failing:**
   - Check service registration in registry
   - Verify nginx routing configuration
   - Check API server logs

### Debug Commands

```bash
# Check service status
curl http://localhost:3001/services

# Test API endpoint
curl http://localhost/api/status

# Check WebSocket connection
wscat -c ws://localhost:3002

# View service logs
docker-compose logs api-server
```

## Development

### Adding New Endpoints

1. **API Server** (api-server/server.js):
   ```javascript
   app.get('/api/new-endpoint', (req, res) => {
     // Implementation
   });
   ```

2. **Update Service Registry** dengan endpoint baru

3. **Frontend Integration** di static server

### Testing

```bash
# Run tests for each service
cd service-registry && npm test
cd api-server && npm test
cd static-server && npm test
```

## License

MIT License - See LICENSE file for details.
