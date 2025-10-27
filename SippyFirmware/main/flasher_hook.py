#!/usr/bin/env python3
import serial
import time
import os

PORT = "/dev/ttyACM0"
BAUD = 115200

def wait_for_device(port):
    print(f"Waiting for {port} to appear...")
    while not os.path.exists(port):
        time.sleep(0.1)
    print(f"{port} detected.")

def enter_bootloader(port, baud):
    print("Opening serial port...")
    with serial.Serial(port, baud, timeout=1) as ser:
        ser.rts = True
        ser.dtr = True
        time.sleep(0.1)

        # Step 3: Release reset (EN high)
        ser.dtr = False
        ser.rts = False
        time.sleep(0.1)

        ser.dtr = True
        ser.rts = True
        time.sleep(0.1)

        ser.dtr = False
        ser.rts = False
        time.sleep(0.1)

        print("ESP32 should now be in bootloader mode.")
        # Keep port open briefly to hold state

if __name__ == "__main__":
    wait_for_device(PORT)
    try:
        enter_bootloader(PORT, BAUD)
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    print("Waiting for next wake-up...\n")