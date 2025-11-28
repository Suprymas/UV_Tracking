# Local Setup Guide (Without Docker)

This guide shows how to run the UV Tracking system locally without Docker.

## Prerequisites

1. **Python 3.11+** - Already installed (you have `.venv`)
2. **Redis** - Need to install and run
3. **Virtual Environment** - Already set up

## Step 1: Install Redis

### macOS (using Homebrew)
```bash
brew install redis
brew services start redis
```

### Verify Redis is running
```bash
redis-cli ping
# Should return: PONG
```

## Step 2: Install Python Dependencies

```bash
# Activate your virtual environment (if not already active)
source .venv/bin/activate

# Install Django app dependencies
pip install -r requirements.txt

# Install WebSocket server dependencies
cd AS7265/websocket_server
pip install -r requirements.txt
cd ../..
```

## Step 3: Setup Environment Variables

Create `.env` file in `djangoapp/`:
```bash
cd djangoapp/
cat > .env << EOF
UV_API_KEY="your_openuv_api_key_here"
REDIS_URL="redis://localhost:6379/0"
EOF
cd ..
```

Get your API key from: https://www.openuv.io

## Step 4: Setup Django Database

```bash
cd djangoapp/
python manage.py migrate
cd ..
```

## Step 5: Run All Services

You'll need **4 terminal windows**:

### Terminal 1: Django Server
```bash
cd djangoapp/
python manage.py runserver
```
Access at: http://127.0.0.1:8000/

### Terminal 2: Celery Worker
```bash
cd djangoapp/
celery -A uvsite worker --loglevel=info
```

### Terminal 3: Celery Beat (Scheduler)
```bash
cd djangoapp/
celery -A uvsite beat --loglevel=info
```

### Terminal 4: WebSocket Server
```bash
cd AS7265/websocket_server
python -m websocket_server.main
```
Or use the legacy entry point:
```bash
cd AS7265
python WebSocket.py
```

## Quick Start Script

You can also create a simple script to start all services. Create `start_local.sh`:

```bash
#!/bin/bash

# Start Redis (if not already running)
brew services start redis 2>/dev/null || redis-server --daemonize yes

# Activate virtual environment
source .venv/bin/activate

# Start Django
cd djangoapp/
python manage.py runserver &
DJANGO_PID=$!
cd ..

# Start Celery Worker
cd djangoapp/
celery -A uvsite worker --loglevel=info &
WORKER_PID=$!
cd ..

# Start Celery Beat
cd djangoapp/
celery -A uvsite beat --loglevel=info &
BEAT_PID=$!
cd ..

# Start WebSocket Server
cd AS7265/websocket_server
python -m websocket_server.main &
WS_PID=$!
cd ../..

echo "All services started!"
echo "Django: http://127.0.0.1:8000/"
echo "WebSocket: ws://127.0.0.1:8765"
echo ""
echo "Press Ctrl+C to stop all services"
echo "PIDs: Django=$DJANGO_PID, Worker=$WORKER_PID, Beat=$BEAT_PID, WS=$WS_PID"

# Wait for interrupt
trap "kill $DJANGO_PID $WORKER_PID $BEAT_PID $WS_PID 2>/dev/null; exit" INT TERM
wait
```

Make it executable:
```bash
chmod +x start_local.sh
./start_local.sh
```

## Troubleshooting

### Redis Connection Error
```bash
# Check if Redis is running
redis-cli ping

# Start Redis manually
redis-server
```

### Port Already in Use
```bash
# Find process using port 8000
lsof -i :8000

# Find process using port 8765
lsof -i :8765

# Kill the process
kill -9 <PID>
```

### Module Not Found Errors
Make sure you've installed all dependencies:
```bash
pip install -r requirements.txt
cd AS7265/websocket_server && pip install -r requirements.txt && cd ../..
```

