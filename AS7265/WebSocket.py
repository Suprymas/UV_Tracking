#!/usr/bin/env python3


import asyncio
import websockets
import json
from datetime import datetime

# Store connected clients
connected_clients = set()


async def handle_client(websocket):
    connected_clients.add(websocket)
    client_ip = websocket.remote_address[0]
    print(f"[{datetime.now()}] Client connected: {client_ip}")

    try:
        async for message in websocket:
            print(f"[{datetime.now()}] Received from {client_ip}: {message}")

            try:
                data = json.loads(message)

                # Process different message types
                if data.get("type") == "sensor":
                    print(f"  Sensor data - Temp: {data.get('temperature')}°C, Humidity: {data.get('humidity')}%")

                    # Send acknowledgment back to ESP32
                    response = {
                        "type": "ack",
                        "status": "received",
                        "timestamp": datetime.now().isoformat()
                    }
                    await websocket.send(json.dumps(response))

                elif data.get("type") == "status":
                    print(f"  Status: {data.get('message')}")

            except json.JSONDecodeError:
                print(f"  Non-JSON message: {message}")

    except websockets.exceptions.ConnectionClosed:
        print(f"[{datetime.now()}] Client disconnected: {client_ip}")
    finally:
        connected_clients.remove(websocket)


async def send_commands():
    while True:
        await asyncio.sleep(10)  # Send command every 10 seconds

        if connected_clients:
            command = {
                "type": "command",
                "action": "read_sensor",
                "timestamp": datetime.now().isoformat()
            }

            # Send to all connected clients
            disconnected = set()
            for client in connected_clients:
                try:
                    await client.send(json.dumps(command))
                    print(f"[{datetime.now()}] Sent command to {client.remote_address[0]}")
                except:
                    disconnected.add(client)

            # Remove disconnected clients
            connected_clients.difference_update(disconnected)


async def main():
    # Start server on all interfaces, port 8765
    server = await websockets.serve(handle_client, "0.0.0.0", 8765, ping_interval=None)
    print(f"[{datetime.now()}] WebSocket server started on ws://0.0.0.0:8765")
    print("Waiting for ESP32-C3 connections...")

    # Start command sender task
    asyncio.create_task(send_commands())

    # Keep server running
    await asyncio.Future()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nServer stopped")