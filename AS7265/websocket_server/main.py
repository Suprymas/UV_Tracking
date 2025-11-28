#!/usr/bin/env python3
"""Entry point for the WebSocket server"""
import asyncio
import sys
from .server import main

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nServer stopped")
        sys.exit(0)

