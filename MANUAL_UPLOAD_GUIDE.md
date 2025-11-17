# Manual Upload Guide for ESP32

This guide explains how to manually upload code from the web editor to your ESP32 device.

## Method 1: Using Arduino IDE (Recommended for Arduino/C++ code)

### Step 1: Get Your Code
1. Open the web editor at `http://127.0.0.1:8000/esp32connection/editor/`
2. Load or write your ESP32 code
3. Click **"Copy Code"** button to copy the code to clipboard
   - OR click **"Download"** to download as `.ino` file

### Step 2: Setup Arduino IDE
1. **Install Arduino IDE** (if not already installed):
   ```bash
   # macOS
   brew install --cask arduino
   ```

2. **Install ESP32 Board Support**:
   - Open Arduino IDE
   - Go to `File` → `Preferences`
   - Add this URL to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to `Tools` → `Board` → `Boards Manager`
   - Search for "ESP32" and install "esp32 by Espressif Systems"

3. **Select Your Board**:
   - Go to `Tools` → `Board` → `ESP32 Arduino`
   - Select your ESP32 model (e.g., "ESP32 Dev Module")

4. **Select Port**:
   - Go to `Tools` → `Port`
   - Select your ESP32 port (e.g., `/dev/cu.usbmodem1301`)

### Step 3: Upload Code
1. **Paste Code**:
   - Create a new sketch in Arduino IDE (`File` → `New`)
   - Paste your copied code (or open the downloaded `.ino` file)

2. **Verify** (optional):
   - Click the checkmark button to verify/compile the code

3. **Upload**:
   - Click the arrow button (→) to upload
   - Wait for "Done uploading" message

## Method 2: Using PlatformIO (Recommended for advanced users)

### Step 1: Install PlatformIO
```bash
# Install PlatformIO Core
pip install platformio

# Or install PlatformIO IDE extension in VS Code
```

### Step 2: Create Project
```bash
# Create new project
pio project init --board esp32dev

# Or use existing project structure
cd your_project
pio init --board esp32dev
```

### Step 3: Add Your Code
1. Copy code from web editor
2. Paste into `src/main.cpp` (for C++) or `src/main.ino` (for Arduino)

### Step 4: Upload
```bash
# Upload to ESP32
pio run --target upload

# Or with port specified
pio run --target upload --upload-port /dev/cu.usbmodem1301
```

## Method 3: Using esptool.py (For compiled binaries)

### Step 1: Install esptool
```bash
pip install esptool
```

### Step 2: Compile Code First
- Use Arduino IDE or PlatformIO to compile your code
- This will generate a `.bin` file

### Step 3: Upload Binary
```bash
# Erase flash
esptool.py --port /dev/cu.usbmodem1301 erase_flash

# Write firmware
esptool.py --port /dev/cu.usbmodem1301 write_flash 0x1000 firmware.bin
```

## Method 4: Using Arduino CLI (Command Line)

### Step 1: Install Arduino CLI
```bash
# macOS
brew install arduino-cli
```

### Step 2: Setup
```bash
# Initialize config
arduino-cli config init

# Add ESP32 board URL
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

# Update board index
arduino-cli core update-index

# Install ESP32 core
arduino-cli core install esp32:esp32
```

### Step 3: Upload
```bash
# Compile and upload
arduino-cli compile --fqbn esp32:esp32:esp32 your_sketch.ino
arduino-cli upload -p /dev/cu.usbmodem1301 --fqbn esp32:esp32:esp32 your_sketch.ino
```

## Quick Reference

### Find Your ESP32 Port
```bash
# macOS/Linux
ls /dev/cu.* | grep -i usb
ls /dev/tty.* | grep -i usb

# Common ports:
# macOS: /dev/cu.usbmodem1301, /dev/cu.SLAB_USBtoUART
# Linux: /dev/ttyUSB0, /dev/ttyACM0
```

### Common Upload Settings
- **Baud Rate**: 115200 (default)
- **Flash Size**: 4MB (most ESP32 boards)
- **Upload Speed**: 921600 (faster) or 115200 (slower, more reliable)

### Troubleshooting

**Port not found:**
- Make sure ESP32 is connected via USB
- Check USB cable (some cables are power-only)
- Try different USB port

**Upload failed:**
- Press and hold BOOT button on ESP32 during upload
- Try lower upload speed (115200)
- Check if another program is using the port

**Permission denied:**
```bash
# macOS - add user to dialout group (usually not needed)
# Linux
sudo usermod -a -G dialout $USER
# Then logout and login again
```

## Web Editor Features

- **Copy Code**: Click "Copy Code" to copy all code to clipboard
- **Download**: Click "Download" to save code as file (.ino, .c, or .py)
- **Save**: Save code to database for later use
- **Load**: Load previously saved code

## Next Steps

After uploading:
1. Open Serial Monitor to see ESP32 output
2. Use the serial monitor tool: `python esp32_serial_monitor.py -p /dev/cu.usbmodem1301`
3. Or use Arduino IDE Serial Monitor (Tools → Serial Monitor)

