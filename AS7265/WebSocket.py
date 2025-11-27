#!/usr/bin/env python3


import asyncio
import websockets
import json
from datetime import datetime

# Store connected clients
connected_clients = set()

async def broadcast_to_web_clients(data, sender=None):
    if not connected_clients:
        return

    disconnected = set()
    for ws in connected_clients:
        if ws == sender:
            continue
        try:
            await ws.send(json.dumps(data))
        except:
            disconnected.add(ws)

    connected_clients.difference_update(disconnected)

async def send_command_to_all(action: str, sender=None):
    """
    Broadcast a command to all connected clients.
    action: string, e.g., "read_sensor", "debug_on", "debug_off"
    """
    if not connected_clients:
        return

    command = {
        "type": "command",
        "action": action,
        "timestamp": datetime.now().isoformat()
    }

    disconnected = set()
    for client in connected_clients:
        if client == sender:
            continue
        try:
            await client.send(json.dumps(command))
            print(f"[{datetime.now()}] Sent {action} to {client.remote_address[0]}")
        except:
            disconnected.add(client)

    # Remove disconnected clients
    connected_clients.difference_update(disconnected)


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
                    await broadcast_to_web_clients(data, sender=websocket)


                elif data.get("type") == "command":
                    action = data.get("action")
                    if action in ["read_sensor", "debug_on", "debug_off"]:
                        await send_command_to_all(action, sender=websocket)

            except json.JSONDecodeError:
                print(f"  Non-JSON message: {message}")

    except websockets.exceptions.ConnectionClosed:
        print(f"[{datetime.now()}] Client disconnected: {client_ip}")
    finally:
        connected_clients.remove(websocket)



async def main():
    # Start server on all interfaces, port 8765
    server = await websockets.serve(handle_client, "0.0.0.0", 8765, ping_interval=None)
    print(f"[{datetime.now()}] WebSocket server started on ws://0.0.0.0:8765")
    print("Waiting for ESP32-C3 connections...")


    # Keep server running
    await asyncio.Future()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nServer stopped")