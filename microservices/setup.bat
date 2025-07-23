@echo off
echo Starting IoT Weighing Scale Microservices...
echo.

REM Check if Docker is running
docker info >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Docker is not running. Please start Docker Desktop first.
    pause
    exit /b 1
)

echo Installing dependencies for all services...
echo.

REM Install dependencies for each service
echo Installing Service Registry dependencies...
cd service-registry
call npm install
cd ..

echo Installing API Server dependencies...
cd api-server
call npm install
cd ..

echo Installing Static Server dependencies...
cd static-server
call npm install
cd ..

echo.
echo Building and starting all services with Docker Compose...
docker-compose up --build -d

echo.
echo Waiting for services to start...
timeout /t 10 /nobreak >nul

echo.
echo Checking service status...
echo.

REM Check each service
echo Service Registry:
curl -s http://localhost:3001/health >nul 2>&1
if %errorlevel% equ 0 (
    echo ✓ Service Registry is running on http://localhost:3001
) else (
    echo ✗ Service Registry is not responding
)

echo API Server:
curl -s http://localhost:3000/health >nul 2>&1
if %errorlevel% equ 0 (
    echo ✓ API Server is running on http://localhost:3000
) else (
    echo ✗ API Server is not responding
)

echo Static Server:
curl -s http://localhost:8080/health >nul 2>&1
if %errorlevel% equ 0 (
    echo ✓ Static Server is running on http://localhost:8080
) else (
    echo ✗ Static Server is not responding
)

echo Load Balancer:
curl -s http://localhost:80/health >nul 2>&1
if %errorlevel% equ 0 (
    echo ✓ Nginx Load Balancer is running on http://localhost:80
) else (
    echo ✗ Nginx Load Balancer is not responding
)

echo.
echo ========================================
echo IoT Weighing Scale Microservices Setup Complete!
echo ========================================
echo.
echo Access Points:
echo - Main Application: http://localhost
echo - Dashboard: http://localhost/dashboard
echo - Configuration: http://localhost/config
echo - API Endpoints: http://localhost/api/*
echo - Service Registry: http://localhost:3001
echo.
echo To stop all services: docker-compose down
echo To view logs: docker-compose logs -f
echo.
pause
