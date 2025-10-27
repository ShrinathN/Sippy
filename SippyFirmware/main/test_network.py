#!/bin/python3

from socket import *
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
from collections import deque

LENGTH_BUFFER = 250

acc_x_buffer = deque(maxlen=LENGTH_BUFFER)
acc_y_buffer = deque(maxlen=LENGTH_BUFFER)
acc_z_buffer = deque(maxlen=LENGTH_BUFFER)
gyro_x_buffer = deque(maxlen=LENGTH_BUFFER)
gyro_y_buffer = deque(maxlen=LENGTH_BUFFER)
gyro_z_buffer = deque(maxlen=LENGTH_BUFFER)



s = socket(AF_INET, SOCK_DGRAM)
s.bind(('0.0.0.0', 8080))


def socket_get_data():
    return s.recv(10000)

def get_data_from_line(txt_line):
    try:
        x = str(txt_line, 'utf-8').split()
        acc_x = float(x[0])
        acc_y = float(x[1])
        acc_z = float(x[2])
        gyro_x = float(x[3])
        gyro_y = float(x[4])
        gyro_z = float(x[5])
        print(acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z)
        return acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z
    except (ValueError, IndexError) as e:
        print(f"Parse error: {e}")
        return None


fig, ax = plt.subplots(2, 1)
fig.suptitle('LSM6DSL Sensor Data')

# Create line objects once
gyro_x_line, = ax[0].plot([], [], label='Gyro X', color='red')
gyro_y_line, = ax[0].plot([], [], label='Gyro Y', color='green')
gyro_z_line, = ax[0].plot([], [], label='Gyro Z', color='blue')

acc_x_line, = ax[1].plot([], [], label='Accel X', color='red')
acc_y_line, = ax[1].plot([], [], label='Accel Y', color='green')
acc_z_line, = ax[1].plot([], [], label='Accel Z', color='blue')

# Configure axes
ax[0].set_ylabel('Angular Velocity (°/s)')
ax[0].set_title('Gyroscope')
ax[0].set_xlim(0, LENGTH_BUFFER - 1)
ax[0].set_ylim(-251.0,251.0)
ax[0].legend(loc='upper left')
ax[0].grid()

ax[1].set_xlabel('Sample')
ax[1].set_ylabel('Acceleration (m/s²)')
ax[1].set_title('Accelerometer')
ax[1].set_xlim(0, LENGTH_BUFFER - 1)
ax[1].set_ylim(-1.1, 1.1)
ax[1].legend(loc='upper left')
ax[1].grid()

def animation_function(frame):
    try:
        data = socket_get_data()
        result = get_data_from_line(data)
        
        if result is None:
            return gyro_x_line, gyro_y_line, gyro_z_line, acc_x_line, acc_y_line, acc_z_line
        
        acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z = result
        
        # Update buffers
        acc_x_buffer.append(acc_x)
        acc_y_buffer.append(acc_y)
        acc_z_buffer.append(acc_z)
        gyro_x_buffer.append(gyro_x)
        gyro_y_buffer.append(gyro_y)
        gyro_z_buffer.append(gyro_z)
        
        # Update line data
        gyro_x_line.set_data(np.arange(len(gyro_x_buffer)), gyro_x_buffer)
        gyro_y_line.set_data(np.arange(len(gyro_y_buffer)), gyro_y_buffer)
        gyro_z_line.set_data(np.arange(len(gyro_z_buffer)), gyro_z_buffer)
        
        acc_x_line.set_data(np.arange(len(acc_x_buffer)), acc_x_buffer)
        acc_y_line.set_data(np.arange(len(acc_y_buffer)), acc_y_buffer)
        acc_z_line.set_data(np.arange(len(acc_z_buffer)), acc_z_buffer)
        
    except Exception as e:
        print(f"Error in animation: {e}")
    
    return gyro_x_line, gyro_y_line, gyro_z_line, acc_x_line, acc_y_line, acc_z_line

fa = FuncAnimation(fig, animation_function, interval=0, blit=True)
plt.show()