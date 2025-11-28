#!/bin/bash

# UV Tracking - Local Startup Script
# This script starts all services locally without Docker

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Starting UV Tracking System (Local Mode)${NC}"
echo ""

# Check if virtual environment is activated
if [[ -z "$VIRTUAL_ENV" ]]; then
    echo -e "${YELLOW}Warning: Virtual environment not activated${NC}"
    echo "Activating .venv..."
    source .venv/bin/activate
fi

# Check if Redis is running
echo -e "${GREEN}Checking Redis...${NC}"
if ! redis-cli ping > /dev/null 2>&1; then
    echo -e "${YELLOW}Redis is not running. Attempting to start...${NC}"
    if command -v brew &> /dev/null; then
        brew services start redis 2>/dev/null || redis-server --daemonize yes
    else
        redis-server --daemonize yes
    fi
    sleep 2
    if ! redis-cli ping > /dev/null 2>&1; then
        echo -e "${RED}Error: Could not start Redis. Please start it manually.${NC}"
        exit 1
    fi
fi
echo -e "${GREEN}✓ Redis is running${NC}"

# Check if .env file exists
if [ ! -f "djangoapp/.env" ]; then
    echo -e "${YELLOW}Warning: djangoapp/.env not found${NC}"
    echo "Creating template .env file..."
    cat > djangoapp/.env << EOF
UV_API_KEY=""
REDIS_URL="redis://localhost:6379/0"
EOF
    echo -e "${YELLOW}Please edit djangoapp/.env and add your UV_API_KEY${NC}"
fi

# Create logs directory
mkdir -p logs

# Function to cleanup on exit
cleanup() {
    echo ""
    echo -e "${YELLOW}Stopping all services...${NC}"
    kill $DJANGO_PID $WORKER_PID $BEAT_PID $WS_PID 2>/dev/null || true
    echo -e "${GREEN}All services stopped${NC}"
    exit 0
}

trap cleanup INT TERM

# Start Django
echo -e "${GREEN}Starting Django server...${NC}"
cd djangoapp/
python manage.py runserver > ../logs/django.log 2>&1 &
DJANGO_PID=$!
cd ..
echo -e "${GREEN}✓ Django started (PID: $DJANGO_PID)${NC}"

# Start Celery Worker
echo -e "${GREEN}Starting Celery worker...${NC}"
cd djangoapp/
celery -A uvsite worker --loglevel=info > ../logs/celery_worker.log 2>&1 &
WORKER_PID=$!
cd ..
echo -e "${GREEN}✓ Celery worker started (PID: $WORKER_PID)${NC}"

# Start Celery Beat
echo -e "${GREEN}Starting Celery beat...${NC}"
cd djangoapp/
celery -A uvsite beat --loglevel=info > ../logs/celery_beat.log 2>&1 &
BEAT_PID=$!
cd ..
echo -e "${GREEN}✓ Celery beat started (PID: $BEAT_PID)${NC}"

# Start WebSocket Server
echo -e "${GREEN}Starting WebSocket server...${NC}"
# Run from project root with PYTHONPATH set to project root
PYTHONPATH="$(pwd):$PYTHONPATH" python -m AS7265.websocket_server.main > logs/websocket.log 2>&1 &
WS_PID=$!
echo -e "${GREEN}✓ WebSocket server started (PID: $WS_PID)${NC}"

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}All services started successfully!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Services:"
echo "  • Django:      http://127.0.0.1:8000/"
echo "  • WebSocket:   ws://127.0.0.1:8765"
echo ""
echo "Logs are in the 'logs/' directory"
echo ""
echo "Press Ctrl+C to stop all services"
echo ""

# Wait for all background processes
wait

