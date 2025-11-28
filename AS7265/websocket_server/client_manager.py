"""Client connection manager for WebSocket server"""
import logging
from typing import Set
import websockets
from datetime import datetime

logger = logging.getLogger(__name__)


class ClientManager:
    """Manages WebSocket client connections"""
    
    def __init__(self):
        self.clients: Set[websockets.WebSocketServerProtocol] = set()
    
    def add_client(self, websocket: websockets.WebSocketServerProtocol) -> None:
        """Add a client to the connection pool"""
        self.clients.add(websocket)
        client_ip = websocket.remote_address[0] if websocket.remote_address else "unknown"
        logger.info(f"Client connected: {client_ip} (Total: {len(self.clients)})")
    
    def remove_client(self, websocket: websockets.WebSocketServerProtocol) -> None:
        """Remove a client from the connection pool"""
        if websocket in self.clients:
            self.clients.remove(websocket)
            client_ip = websocket.remote_address[0] if websocket.remote_address else "unknown"
            logger.info(f"Client disconnected: {client_ip} (Total: {len(self.clients)})")
    
    def get_client_count(self) -> int:
        """Get the number of connected clients"""
        return len(self.clients)
    
    async def broadcast(self, data: dict, sender: websockets.WebSocketServerProtocol = None) -> None:
        """
        Broadcast data to all connected clients except the sender
        
        Args:
            data: Dictionary to send as JSON
            sender: Optional sender websocket to exclude from broadcast
        """
        if not self.clients:
            return
        
        import json
        message = json.dumps(data)
        disconnected = set()
        
        for client in self.clients:
            if client == sender:
                continue
            try:
                await client.send(message)
            except Exception as e:
                logger.warning(f"Failed to send to client: {e}")
                disconnected.add(client)
        
        # Clean up disconnected clients
        for client in disconnected:
            self.remove_client(client)
    
    def get_all_clients(self) -> Set[websockets.WebSocketServerProtocol]:
        """Get all connected clients"""
        return self.clients.copy()

