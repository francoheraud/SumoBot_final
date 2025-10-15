"""
Reading raw data from /dev/ttyACM0 port for graphing purposes.
- Franco H
"""

import time
import serial
import matplotlib.pyplot as plt

ser = serial.Serial(port='/dev/ttyACM0', baudrate=115200, timeout=1)
time.sleep(2)  # wait for ESP32 to reboot
ser.reset_input_buffer()

plt.ion()
fig, ax = plt.subplots()
ax.set_title("TT DC Motor PI Controller")
ax.set_xlabel("Samples")
ax.set_ylabel("Velocity (Ticks/s)")

desiredVelocity, actualVelocity, xData = [], [], []
line1, = ax.plot([], [], label="Desired Velocity")
line2, = ax.plot([], [], label="Actual Velocity")
ax.legend()
ax.set_ylim(bottom=0, top=30)

inc = 0

try:
    while True:
        try:
            line = ser.readline().decode().strip()
            if not line:
                continue

            floatData = [float(x) for x in line.split(',')]
            if len(floatData) < 2:
                continue

            inc += 1
            if inc == 500:
                inc = 0
                xData.clear()
                desiredVelocity.clear()
                actualVelocity.clear()

            xData.append(inc)
            desiredVelocity.append(floatData[0])
            actualVelocity.append(floatData[1])

            line1.set_data(xData, desiredVelocity)
            line2.set_data(xData, actualVelocity)

            ax.relim()
            ax.autoscale_view(tight=True, scalex=True, scaley=True)
            fig.canvas.draw()
            fig.canvas.flush_events()
            
        except ValueError:
            continue

except KeyboardInterrupt:
    print("Closing serial connection...")
    ser.close()
