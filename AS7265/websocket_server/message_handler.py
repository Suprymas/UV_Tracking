"""Message handler for processing WebSocket messages"""
import json
import logging
from datetime import datetime
from typing import Dict, Any
import websockets
from .client_manager import ClientManager
from .config import config

logger = logging.getLogger(__name__)


class MessageHandler:
    """Handles incoming WebSocket messages"""
    
    def __init__(self, client_manager: ClientManager):
        self.client_manager = client_manager
    
    async def handle_message(self, message: str, websocket: websockets.WebSocketServerProtocol) -> None:
        """
        Process an incoming message from a client
        
        Args:
            message: Raw message string
            websocket: The websocket connection that sent the message
        """
        client_ip = websocket.remote_address[0] if websocket.remote_address else "unknown"
        logger.debug(f"Received from {client_ip}: {message}")
        
        try:
            data = json.loads(message)
            message_type = data.get("type")
            
            if message_type == "sensor":
                await self._handle_sensor_data(data, websocket)
            elif message_type == "command":
                await self._handle_command(data, websocket)
            else:
                logger.warning(f"Unknown message type: {message_type}")
        
        except json.JSONDecodeError as e:
            logger.warning(f"Non-JSON message from {client_ip}: {message[:100]}")
        except Exception as e:
            logger.error(f"Error handling message: {e}", exc_info=True)
    
    async def _handle_sensor_data(self, data: Dict[str, Any], sender: websockets.WebSocketServerProtocol) -> None:
        """Handle sensor data messages - broadcast to all other clients"""
        await self.client_manager.broadcast(data, sender=sender)
        logger.debug("Broadcasted sensor data to all clients")
    
    async def _handle_command(self, data: Dict[str, Any], sender: websockets.WebSocketServerProtocol) -> None:
        """Handle command messages - validate and broadcast to all clients"""
        action = data.get("action")
        
        if action not in config.ALLOWED_COMMANDS:
            logger.warning(f"Invalid command: {action}")
            return
        
        command = {
            "type": "command",
            "action": action,
            "timestamp": datetime.now().isoformat()
        }
        
        await self.client_manager.broadcast(command, sender=sender)
        client_ip = sender.remote_address[0] if sender.remote_address else "unknown"
        logger.info(f"Broadcasted command '{action}' to all clients (from {client_ip})")

