# WebSocket Server

Modular WebSocket server for the UV Tracking system. Handles communication between ESP32 devices and web clients.

## Features

- Modular architecture with separated concerns
- Client connection management
- Message routing and broadcasting
- Configurable via environment variables
- Docker support
- Graceful shutdown handling

## Module Structure

- `config.py` - Configuration management
- `client_manager.py` - Client connection pool management
- `message_handler.py` - Message processing and routing
- `server.py` - Main server implementation
- `main.py` - Entry point

## Configuration

The server can be configured using environment variables:

- `WS_HOST` - Server host (default: `0.0.0.0`)
- `WS_PORT` - Server port (default: `8765`)
- `WS_LOG_LEVEL` - Logging level: DEBUG, INFO, WARNING, ERROR (default: `INFO`)
- `WS_PING_INTERVAL` - Ping interval in seconds (default: `20`, set to `0` to disable)
- `WS_MAX_CONNECTIONS` - Maximum concurrent connections (default: `100`)

## Running Locally

1. **Install dependencies**
   ```bash
   pip install -r requirements.txt
   ```

2. **Run the server**
   ```bash
   python -m websocket_server.main
   ```
   
   Or use the legacy entry point:
   ```bash
   python ../WebSocket.py
   ```

## Running with Docker

The server is included in the main `docker-compose.yml`. To run it separately:

```bash
cd AS7265/websocket_server
docker build -t uv-websocket .
docker run -p 8765:8765 -e WS_LOG_LEVEL=DEBUG uv-websocket
```

## Message Protocol

### Sensor Data (from ESP32)
```json
{
  "type": "sensor",
  "readings": [0.0, 0.0, ...],  // 18 channel values
  "mode": "debug"  // optional
}
```

### Commands (to ESP32)
```json
{
  "type": "command",
  "action": "read_sensor",  // or "debug_on", "debug_off"
  "timestamp": "2024-01-01T12:00:00"
}
```

### Status Messages
```json
{
  "type": "status",
  "message": "ESP connected"
}
```

## Allowed Commands

- `read_sensor` - Request sensor reading
- `debug_on` - Enable debug mode (continuous readings)
- `debug_off` - Disable debug mode

