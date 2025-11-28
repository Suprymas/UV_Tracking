"""Main WebSocket server implementation"""
import asyncio
import logging
import signal
import sys
import websockets
from websockets.exceptions import ConnectionClosed
from .config import config
from .client_manager import ClientManager
from .message_handler import MessageHandler

# Configure logging
logging.basicConfig(
    level=getattr(logging, config.LOG_LEVEL),
    format='[%(asctime)s] %(levelname)s - %(name)s - %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)


class WebSocketServer:
    """WebSocket server for UV Tracking system"""
    
    def __init__(self):
        self.client_manager = ClientManager()
        self.message_handler = MessageHandler(self.client_manager)
        self.server = None
        self._shutdown_event = asyncio.Event()
    
    async def handle_client(self, websocket: websockets.WebSocketServerProtocol, path: str) -> None:
        """
        Handle a new client connection
        
        Args:
            websocket: The websocket connection
            path: The path requested by the client
        """
        self.client_manager.add_client(websocket)
        client_ip = websocket.remote_address[0] if websocket.remote_address else "unknown"
        
        try:
            async for message in websocket:
                await self.message_handler.handle_message(message, websocket)
        
        except ConnectionClosed:
            logger.info(f"Connection closed: {client_ip}")
        except Exception as e:
            logger.error(f"Error in client handler: {e}", exc_info=True)
        finally:
            self.client_manager.remove_client(websocket)
    
    async def start(self) -> None:
        """Start the WebSocket server"""
        logger.info(f"Starting WebSocket server on {config.server_url}")
        logger.info("Waiting for ESP32-C3 connections...")
        
        self.server = await websockets.serve(
            self.handle_client,
            config.HOST,
            config.PORT,
            ping_interval=config.PING_INTERVAL if config.PING_INTERVAL > 0 else None
        )
        
        logger.info(f"WebSocket server started successfully on {config.server_url}")
        logger.info(f"Server is ready to accept connections")
        
        # Wait for shutdown signal
        await self._shutdown_event.wait()
    
    async def stop(self) -> None:
        """Stop the WebSocket server gracefully"""
        logger.info("Shutting down WebSocket server...")
        
        if self.server:
            self.server.close()
            await self.server.wait_closed()
        
        # Close all client connections
        for client in list(self.client_manager.get_all_clients()):
            try:
                await client.close()
            except Exception as e:
                logger.warning(f"Error closing client: {e}")
        
        logger.info("WebSocket server stopped")
    
    def signal_shutdown(self) -> None:
        """Signal the server to shutdown"""
        self._shutdown_event.set()


async def main():
    """Main entry point for the server"""
    server = WebSocketServer()
    
    # Setup signal handlers for graceful shutdown
    def signal_handler(sig, frame):
        logger.info("Received shutdown signal")
        server.signal_shutdown()
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    try:
        await server.start()
    except KeyboardInterrupt:
        logger.info("Received keyboard interrupt")
        server.signal_shutdown()
    finally:
        await server.stop()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except Exception as e:
        logger.error(f"Fatal error: {e}", exc_info=True)
        sys.exit(1)

