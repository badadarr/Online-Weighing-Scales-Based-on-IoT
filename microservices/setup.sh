#!/bin/bash

echo "Starting IoT Weighing Scale Microservices..."
echo

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "Error: Docker is not running. Please start Docker first."
    exit 1
fi

echo "Installing dependencies for all services..."
echo

# Install dependencies for each service
echo "Installing Service Registry dependencies..."
cd service-registry
npm install
cd ..

echo "Installing API Server dependencies..."
cd api-server
npm install
cd ..

echo "Installing Static Server dependencies..."
cd static-server
npm install
cd ..

echo
echo "Building and starting all services with Docker Compose..."
docker-compose up --build -d

echo
echo "Waiting for services to start..."
sleep 10

echo
echo "Checking service status..."
echo

# Function to check service health
check_service() {
    local name=$1
    local url=$2
    
    if curl -s "$url" > /dev/null 2>&1; then
        echo "✓ $name is running on $url"
    else
        echo "✗ $name is not responding"
    fi
}

# Check each service
check_service "Service Registry" "http://localhost:3001/health"
check_service "API Server" "http://localhost:3000/health"
check_service "Static Server" "http://localhost:8080/health"
check_service "Nginx Load Balancer" "http://localhost:80/health"

echo
echo "========================================"
echo "IoT Weighing Scale Microservices Setup Complete!"
echo "========================================"
echo
echo "Access Points:"
echo "- Main Application: http://localhost"
echo "- Dashboard: http://localhost/dashboard"
echo "- Configuration: http://localhost/config"
echo "- API Endpoints: http://localhost/api/*"
echo "- Service Registry: http://localhost:3001"
echo
echo "To stop all services: docker-compose down"
echo "To view logs: docker-compose logs -f"
echo
