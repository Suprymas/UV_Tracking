#!/usr/bin/env python3
import serial
import time
import sys
import struct


class AS7265x_UART:
    def __init__(self, port='/dev/ttyUSB0', baudrate=115200):
        try:
            self.ser = serial.Serial(port, baudrate, timeout=1)
            time.sleep(2)  # Wait for connection to establish
            print(f"Connected to AS7265x on {port}")
        except serial.SerialException as e:
            print(f"Error connecting to {port}: {e}")
            print("Try checking: ls /dev/tty* to find the correct port")
            sys.exit(1)

    def send_command(self, cmd):
        """Send command to sensor"""
        self.ser.write((cmd + '\n').encode())
        time.sleep(0.1)

    def read_response(self):
        """Read response from sensor"""
        response = ""
        while self.ser.in_waiting > 0:
            response += self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
            time.sleep(0.05)
        return response.strip()

    def get_version(self):
        """Get firmware version"""
        self.send_command('ATVERSION')
        return self.read_response()

    def set_gain(self, gain):
        """Set gain: 0=1x, 1=3.7x, 2=16x, 3=64x"""
        self.send_command(f'ATGAIN={gain}')
        return self.read_response()

    def set_integration_time(self, cycles):
        """Set integration time in cycles (2.8ms per cycle)"""
        self.send_command(f'ATINTTIME={cycles}')
        return self.read_response()

    def take_measurement(self):
        """Take a measurement and return all channel values"""
        self.send_command('ATDATA')
        time.sleep(0.5)  # Wait for measurement
        response = self.read_response()
        return response

    def get_calibrated_data(self):
        """Get calibrated spectral data"""
        self.send_command('ATCDATA')
        time.sleep(0.5)
        response = self.read_response()
        return response

    def parse_rgb_from_response(self, response):
        """Parse RGB values from sensor response
        The AS7265x has specific channels for RGB:
        - Channel indices vary, but typically:
        - R: ~610nm (channel around index 10-12)
        - G: ~560nm (channel around index 6-8)
        - B: ~460nm (channel around index 2-4)
        """
        try:
            # Response format may vary - this is a basic parser
            values = [float(x) for x in response.split(',') if x.strip()]

            if len(values) >= 18:
                # Map approximate channels to RGB
                # AS7265x channels: 410, 435, 460, 485, 510, 535, 560, 585, 610, 645, 680, 705, 730, 760, 810, 860, 900, 940 nm
                b = values[2]  # ~460nm (Blue)
                g = values[6]  # ~560nm (Green)
                r = values[8]  # ~610nm (Red)
                return r, g, b
            else:
                return None, None, None
        except Exception as e:
            print(f"Error parsing response: {e}")
            return None, None, None

    def close(self):
        """Close serial connection"""
        if self.ser.is_open:
            self.ser.close()


def main():
    # Initialize sensor - change port if needed
    # Common ports: /dev/ttyUSB0, /dev/ttyACM0, /dev/ttyUSB1
    sensor = AS7265x_UART(port='/dev/ttyUSB0', baudrate=115200)

    # Get version info
    print("Firmware version:", sensor.get_version())

    # Configure sensor
    print("\nConfiguring sensor...")
    print(sensor.set_gain(2))  # 16x gain for indoor light
    print(sensor.set_integration_time(100))  # 100 cycles

    print("\nReading RGB values (press Ctrl+C to exit)...\n")

    try:
        while True:
            # Get calibrated data
            response = sensor.get_calibrated_data()

            # Parse RGB values
            r, g, b = sensor.parse_rgb_from_response(response)

            if r is not None:
                print(f"R: {r:.2f}  G: {g:.2f}  B: {b:.2f}")
            else:
                print(f"Raw response: {response}")

            time.sleep(1)

    except KeyboardInterrupt:
        print("\n\nStopping measurements...")
        sensor.close()
        print("Done!")


if __name__ == "__main__":
    main()