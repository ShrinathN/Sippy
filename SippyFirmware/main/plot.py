import serial
import re
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# Configure this to your actual COM port and baud rate
SERIAL_PORT = '/dev/ttyACM0'  # or 'COM3' on Windows
BAUD_RATE = 115200

# Data storage: sliding window
MAX_LEN = 100
accel_data = {'x': deque(maxlen=MAX_LEN), 'y': deque(maxlen=MAX_LEN), 'z': deque(maxlen=MAX_LEN)}
gyro_data = {'x': deque(maxlen=MAX_LEN), 'y': deque(maxlen=MAX_LEN), 'z': deque(maxlen=MAX_LEN)}

# Regex pattern to extract values from each line
log_pattern = re.compile(
    r"display_data: ([\d\.\-]+)m/s² ([\d\.\-]+)m/s² ([\d\.\-]+)m/s² ([\d\.\-]+)°/s ([\d\.\-]+)°/s ([\d\.\-]+)°/s"
)

# Setup serial connection
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# Plot setup
fig, (accel_ax, gyro_ax) = plt.subplots(2, 1, figsize=(10, 6))
plt.subplots_adjust(hspace=0.5)

def update_plot(frame):
    line = ser.readline().decode('utf-8', errors='ignore').strip()
    match = log_pattern.search(line)
    if match:
        ax, ay, az, gx, gy, gz = map(float, match.groups())
        accel_data['x'].append(ax)
        accel_data['y'].append(ay)
        accel_data['z'].append(az)
        gyro_data['x'].append(gx)
        gyro_data['y'].append(gy)
        gyro_data['z'].append(gz)

        # Update accel plot
        accel_ax.clear()
        accel_ax.plot(accel_data['x'], label='Accel X')
        accel_ax.plot(accel_data['y'], label='Accel Y')
        accel_ax.plot(accel_data['z'], label='Accel Z')
        accel_ax.set_title("Accelerometer (m/s²)")
        accel_ax.set_ylim(-2.5, 2.5)
        accel_ax.legend()
        accel_ax.grid(True)

        # Update gyro plot
        gyro_ax.clear()
        gyro_ax.plot(gyro_data['x'], label='Gyro X')
        gyro_ax.plot(gyro_data['y'], label='Gyro Y')
        gyro_ax.plot(gyro_data['z'], label='Gyro Z')
        gyro_ax.set_title("Gyroscope (°/s)")
        gyro_ax.set_ylim(-300, 300)
        gyro_ax.legend()
        gyro_ax.grid(True)

ani = animation.FuncAnimation(fig, update_plot, interval=50)
plt.show()
