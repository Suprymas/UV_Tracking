#!/usr/bin/env python3
"""
ESP32 Serial Monitor
A simple tool to monitor ESP32 serial port output
"""
import serial
import serial.tools.list_ports
import sys
import time
import argparse


def list_ports():
    """List all available serial ports"""
    print("Available serial ports:")
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("  No serial ports found")
        return []
    
    port_list = []
    for i, port in enumerate(ports, 1):
        print(f"  {i}. {port.device} - {port.description}")
        port_list.append(port.device)
    return port_list


def monitor_serial(port, baudrate=115200):
    """Monitor serial port and print output"""
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"\nConnected to {port} at {baudrate} baud")
        print("Monitoring serial output (Press Ctrl+C to exit)...\n")
        print("-" * 60)
        
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                except UnicodeDecodeError:
                    # Handle binary data
                    data = ser.read(ser.in_waiting)
                    print(f"[Binary data: {len(data)} bytes]")
            
            time.sleep(0.01)  # Small delay to prevent CPU spinning
            
    except serial.SerialException as e:
        print(f"Error: Could not open port {port}")
        print(f"Details: {e}")
        print("\nTrying to list available ports...")
        list_ports()
        sys.exit(1)
    except KeyboardInterrupt:
        print("\n\nDisconnecting...")
        if ser.is_open:
            ser.close()
        print("Done!")
        sys.exit(0)


def main():
    parser = argparse.ArgumentParser(description='ESP32 Serial Monitor')
    parser.add_argument('-p', '--port', type=str, help='Serial port (e.g., /dev/cu.usbmodem1301)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('-l', '--list', action='store_true', help='List available serial ports')
    
    args = parser.parse_args()
    
    if args.list:
        list_ports()
        return
    
    if not args.port:
        # Try to auto-detect ESP32 port
        ports = serial.tools.list_ports.comports()
        esp32_ports = [p.device for p in ports if 'usb' in p.device.lower() or 'USB' in p.description]
        
        if esp32_ports:
            print(f"Auto-detected port: {esp32_ports[0]}")
            args.port = esp32_ports[0]
        else:
            print("Error: No port specified and could not auto-detect")
            print("\nAvailable ports:")
            list_ports()
            print("\nUsage: python esp32_serial_monitor.py -p /dev/cu.usbmodem1301")
            sys.exit(1)
    
    monitor_serial(args.port, args.baudrate)


if __name__ == "__main__":
    main()

