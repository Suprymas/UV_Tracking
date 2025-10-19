from smbus2 import SMBus as smbus
import time


class TCS34725:
    # I2C address
    ADDRESS = 0x29

    # Register addresses
    COMMAND_BIT = 0x80
    ENABLE = 0x00
    ATIME = 0x01
    CONTROL = 0x0F
    ID = 0x12
    STATUS = 0x13
    CDATAL = 0x14
    CDATAH = 0x15
    RDATAL = 0x16
    RDATAH = 0x17
    GDATAL = 0x18
    GDATAH = 0x19
    BDATAL = 0x1A
    BDATAH = 0x1B

    # Enable register values
    ENABLE_PON = 0x01
    ENABLE_AEN = 0x02

    # Integration time options (in ms)
    INTEGRATION_TIME_2_4MS = 0xFF
    INTEGRATION_TIME_24MS = 0xF6
    INTEGRATION_TIME_50MS = 0xEB
    INTEGRATION_TIME_101MS = 0xD5
    INTEGRATION_TIME_154MS = 0xC0
    INTEGRATION_TIME_700MS = 0x00

    # Gain options
    GAIN_1X = 0x00
    GAIN_4X = 0x01
    GAIN_16X = 0x02
    GAIN_60X = 0x03

    def __init__(self, bus_number=1, integration_time=INTEGRATION_TIME_50MS, gain=GAIN_1X):
        """
        Initialize TCS34725 sensor

        Args:
            bus_number: I2C bus number (usually 1 for RPi)
            integration_time: Integration time constant
            gain: Gain constant
        """
        self.bus = smbus.SMBus(bus_number)
        self.integration_time = integration_time
        self.gain = gain

        # Check sensor ID
        sensor_id = self._read_byte(self.ID)
        if sensor_id not in [0x44, 0x4D]:
            raise RuntimeError(f"TCS34725 not found. Got ID: {hex(sensor_id)}")

        # Initialize sensor
        self._write_byte(self.ATIME, self.integration_time)
        self._write_byte(self.CONTROL, self.gain)
        self._enable()

        print("TCS34725 initialized successfully")

    def _write_byte(self, register, value):
        """Write a byte to a register"""
        self.bus.write_byte_data(self.ADDRESS, self.COMMAND_BIT | register, value)

    def _read_byte(self, register):
        """Read a byte from a register"""
        return self.bus.read_byte_data(self.ADDRESS, self.COMMAND_BIT | register)

    def _read_word(self, register):
        """Read a 16-bit word from a register"""
        low = self._read_byte(register)
        high = self._read_byte(register + 1)
        return (high << 8) | low

    def _enable(self):
        """Enable the sensor"""
        self._write_byte(self.ENABLE, self.ENABLE_PON)
        time.sleep(0.003)  # Wait 3ms
        self._write_byte(self.ENABLE, self.ENABLE_PON | self.ENABLE_AEN)

    def disable(self):
        """Disable the sensor"""
        reg = self._read_byte(self.ENABLE)
        self._write_byte(self.ENABLE, reg & ~(self.ENABLE_PON | self.ENABLE_AEN))

    def is_ready(self):
        """Check if data is ready to read"""
        return self._read_byte(self.STATUS) & 0x01

    def get_raw_data(self):
        """
        Read raw RGBC data

        Returns:
            tuple: (red, green, blue, clear) values
        """
        # Wait for valid data
        while not self.is_ready():
            time.sleep(0.001)

        clear = self._read_word(self.CDATAL)
        red = self._read_word(self.RDATAL)
        green = self._read_word(self.GDATAL)
        blue = self._read_word(self.BDATAL)

        return red, green, blue, clear

    def get_rgb(self):
        """
        Get RGB values normalized to 0-255 range

        Returns:
            tuple: (red, green, blue) values (0-255)
        """
        r, g, b, c = self.get_raw_data()

        # Avoid division by zero
        if c == 0:
            return 0, 0, 0

        # Normalize to 0-255
        red = int((r / c) * 255)
        green = int((g / c) * 255)
        blue = int((b / c) * 255)

        # Clamp values
        red = min(255, max(0, red))
        green = min(255, max(0, green))
        blue = min(255, max(0, blue))

        return red, green, blue

    def set_integration_time(self, time_value):
        """Set integration time"""
        self.integration_time = time_value
        self._write_byte(self.ATIME, self.integration_time)

    def set_gain(self, gain_value):
        """Set gain"""
        self.gain = gain_value
        self._write_byte(self.CONTROL, self.gain)


# Example usage
if __name__ == "__main__":
    try:
        # Initialize sensor with default settings
        sensor = TCS34725()

        print("\nReading color data (Press Ctrl+C to stop)...\n")

        while True:
            # Get raw data
            r_raw, g_raw, b_raw, c_raw = sensor.get_raw_data()
            print(f"Raw RGBC: R={r_raw:5d} G={g_raw:5d} B={b_raw:5d} C={c_raw:5d}")

            # Get normalized RGB
            r, g, b = sensor.get_rgb()
            print(f"RGB (0-255): R={r:3d} G={g:3d} B={b:3d}")
            print(f"Hex color: #{r:02X}{g:02X}{b:02X}\n")

            time.sleep(1)

    except KeyboardInterrupt:
        print("\nExiting...")
        sensor.disable()

    except Exception as e:
        print(f"Error: {e}")