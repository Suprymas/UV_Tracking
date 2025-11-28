# UV Tracking Project

A comprehensive UV (ultraviolet) tracking system with ESP32 sensors, Django web application, and WebSocket communication.

## Quick Start Options

### Option 1: Docker Compose (Recommended if Docker is installed)

**Don't have Docker?** See [INSTALL_DOCKER.md](INSTALL_DOCKER.md) for installation instructions.

The easiest way to run the entire system is using Docker Compose:

1. **Create environment file**
   ```bash
   cd djangoapp/
   cp .env.example .env  # or create .env manually
   ```
   
   Edit `.env` and add:
   ```
   UV_API_KEY="your_openuv_api_key_here"
   REDIS_URL="redis://redis:6379/0"
   ```

2. **Start all services**
   ```bash
   docker compose up -d
   ```
   
   This will start:
   - Redis (for Celery)
   - Django web application (port 8000)
   - Celery worker
   - Celery beat scheduler
   - WebSocket server (port 8765)

3. **Access the application**
   - Web UI: http://127.0.0.1:8000/
   - WebSocket: ws://127.0.0.1:8765

4. **View logs**
   ```bash
   docker compose logs -f
   ```

5. **Stop all services**
   ```bash
   docker compose down
   ```
   
   **Note:** If you're using an older version of Docker, you may need to use `docker-compose` (with hyphen) instead of `docker compose`. See [INSTALL_DOCKER.md](INSTALL_DOCKER.md) for details.

### Option 2: Local Setup (Without Docker)

If you don't have Docker installed, you can run everything locally. See [SETUP_LOCAL.md](SETUP_LOCAL.md) for detailed instructions.

**Quick start:**
```bash
# 1. Install Redis
brew install redis
brew services start redis

# 2. Install dependencies
pip install -r requirements.txt
cd AS7265/websocket_server && pip install -r requirements.txt && cd ../..

# 3. Setup .env file in djangoapp/
# Create djangoapp/.env with:
# UV_API_KEY="your_key"
# REDIS_URL="redis://localhost:6379/0"

# 4. Run migrations
cd djangoapp && python manage.py migrate && cd ..

# 5. Start all services (use the script or run manually)
./start_local.sh
```

Or run services manually in separate terminals (see SETUP_LOCAL.md).

## Manual Setup (Development)

### Prerequisites
- Python 3.11+
- Redis (or Docker for Redis)
- Virtual environment

### Setup Steps

1. **Create and activate virtual environment**
   ```bash
   python -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   ```

2. **Install dependencies**
   ```bash
   pip install -r requirements.txt
   ```

3. **Setup environment variables**
   
   Create `.env` file in `djangoapp/`:
   ```
   UV_API_KEY="your_openuv_api_key_here"
   REDIS_URL="redis://localhost:6379/0"
   ```
   
   Get your API key from: https://www.openuv.io

4. **Start Redis**
   ```bash
   docker run -d --rm -p 6379:6379 redis/redis-stack
   ```

5. **Run Django migrations**
   ```bash
   cd djangoapp/
   python manage.py migrate
   ```

6. **Start services** (in separate terminals)
   
   Terminal 1 - Django:
   ```bash
   cd djangoapp/
   python manage.py runserver
   ```
   
   Terminal 2 - Celery Worker:
   ```bash
   cd djangoapp/
   celery -A uvsite worker --loglevel=info
   ```
   
   Terminal 3 - Celery Beat:
   ```bash
   cd djangoapp/
   celery -A uvsite beat --loglevel=info
   ```
   
   Terminal 4 - WebSocket Server:
   ```bash
   cd AS7265/
   python -m websocket_server.main
   # or use the legacy entry point:
   python WebSocket.py
   ```

## Project Structure

- `djangoapp/` - Django web application
  - `dashboard/` - UV data display and API
  - `esp32connection/` - ESP32 code management
  - `uvsite/` - Django project settings

- `AS7265/` - ESP32 and sensor related code
  - `rgbesp/` - ESP32-C3 firmware (ESP-IDF)
  - `websocket_server/` - Modular WebSocket server
  - `WebSocket.py` - Legacy WebSocket server (backward compatible)

## WebSocket Server

The WebSocket server is now modular and can be configured via environment variables:

- `WS_HOST` - Server host (default: 0.0.0.0)
- `WS_PORT` - Server port (default: 8765)
- `WS_LOG_LEVEL` - Logging level (default: INFO)
- `WS_PING_INTERVAL` - Ping interval in seconds (default: 20)
- `WS_MAX_CONNECTIONS` - Maximum connections (default: 100)

## API Endpoints

- `GET /` - Dashboard home
- `GET /api/uv/current` - Get current UV value
- `GET /api/uv/history` - Get UV history (last 48 readings)
- `GET /esp32connection/` - ESP32 code editor

## ESP32 Connection

The ESP32 device connects to the WebSocket server at `ws://<server_ip>:8765` and:
- Sends sensor data (18-channel spectral readings)
- Receives commands (`read_sensor`, `debug_on`, `debug_off`)