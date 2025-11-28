#!/usr/bin/env python3
"""
Legacy WebSocket server entry point.
This file is kept for backward compatibility.
For new deployments, use: python -m websocket_server.main
"""
import sys
import os

# Add the parent directory to the path so we can import the module
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from websocket_server.server import main
import asyncio

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nServer stopped")
        sys.exit(0)