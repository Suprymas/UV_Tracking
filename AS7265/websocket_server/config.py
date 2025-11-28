"""Configuration module for WebSocket server"""
import os
from typing import List


class Config:
    """WebSocket server configuration"""
    
    # Server settings
    HOST: str = os.getenv("WS_HOST", "0.0.0.0")
    PORT: int = int(os.getenv("WS_PORT", "8765"))
    
    # Connection settings
    PING_INTERVAL: int = int(os.getenv("WS_PING_INTERVAL", "20"))  # seconds
    MAX_CONNECTIONS: int = int(os.getenv("WS_MAX_CONNECTIONS", "100"))
    
    # Allowed commands
    ALLOWED_COMMANDS: List[str] = [
        "read_sensor",
        "debug_on",
        "debug_off"
    ]
    
    # Logging
    LOG_LEVEL: str = os.getenv("WS_LOG_LEVEL", "INFO")
    
    @property
    def server_url(self) -> str:
        """Get the full server URL"""
        return f"ws://{self.HOST}:{self.PORT}"


config = Config()

