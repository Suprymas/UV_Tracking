import qwiic_as7265x
import time
import sys


def main():
    sensor = qwiic_as7265x.QwiicAS7265x()

    if not sensor.is_connected():
        print("AS7265x not connected.")
        sys.exit(1)

    print("AS7265x connected successfully!")

    # Initialize the sensor
    sensor.begin()

    # Configure sensor settings
    sensor.set_measurement_mode(3)  # Mode 3: All channels continuous
    sensor.set_gain(3)  # Gain: 0=1x, 1=3.7x, 2=16x, 3=64x
    sensor.set_integration_time(50)  # Integration cycles (2.8ms per cycle)
    sensor.enable_indicator()  # Turn on indicator LED
    sensor.enable_bulb(0)  # AS72651 (white LED) - 0=off, 1-3=brightness
    sensor.enable_bulb(1)  # AS72652 (UV LED)
    sensor.enable_bulb(2)  # AS72653 (IR LED)

    print("\nReading RGB values (press Ctrl+C to exit)...\n")

    try:
        while True:
            # Take measurement
            sensor.take_measurements()

            # Get calibrated RGB values
            # AS7265x has 18 channels, but R, G, B are specific wavelengths
            # R: 610nm, G: 560nm (or 555nm), B: 460nm (or 485nm)

            r = sensor.get_calibrated_r()  # Red (~610nm)
            g = sensor.get_calibrated_g()  # Green (~560nm)
            b = sensor.get_calibrated_b()  # Blue (~460nm)

            print(f"R: {r:.2f}  G: {g:.2f}  B: {b:.2f}")

            # You can also get all 18 channels:
            # channels = sensor.get_calibrated_values()
            # print(f"All channels: {channels}")

            time.sleep(1)

    except KeyboardInterrupt:
        print("\n\nStopping measurements...")
        sensor.disable_bulb(0)
        sensor.disable_bulb(1)
        sensor.disable_bulb(2)
        sensor.disable_indicator()
        print("Done!")


if __name__ == "__main__":
    main()