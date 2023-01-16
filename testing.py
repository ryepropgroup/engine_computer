from labjack import ljm
from time import sleep
import time
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib import style
from collections import deque
import threading
import numpy as np
sensor_name = input("Enter pressure sensor name: ").upper()

handle = ljm.openS("ANY", "ANY", "ANY")
banana = 'banana'

info = ljm.getHandleInfo(handle)

print(
    "Opened a LabJack with Device type: %i, Connection type: %i,\n"
    "Serial number: %i, IP address: %s, Port: %i,\nMax bytes per MB: %i"
    % (info[0], info[1], info[2], ljm.numberToIP(info[3]), info[4], info[5])
)

# Setup and call eReadName to read from AIN0 on the LabJack.
name = "AIN2"
# name = "AIN0_EF_READ_A"
# result = ljm.eReadName(handle, name)
# print("\n%s reading : %f C" % (name, result))
# names = ["AIN0_EF_INDEX", "AIN0_EF_CONFIG_B", "AIN0_EF_CONFIG_D", "AIN0_EF_CONFIG_E", "AIN0_EF_CONFIG_A"]
names = ["AIN2_NEGATIVE_CH", "AIN2_RANGE"]
# aValues = [22, 60052, 1.0, 0.0, 1]
aValues = [3.0, 0.1]

ljm.eWriteNames(handle, len(names), names, aValues)
print(banana)
result = 0.0
results = deque([0],maxlen=100)
def conn():
    global result
    with open(f"logs/{sensor_name}_{int(time.time())}.txt", "w+") as f:
        maxbanana = -1000000
        while True:
            sleep(0.001)
            v = ljm.eReadName(handle, name)
            results.append(v*25000)
            result = sum(results)/100
            maxbanana = max(maxbanana, result)
            print("\n%s live reading : %f" % (name, v*25000))
            print("%s average reading : %f" % (name, result))
            print(f"max is {maxbanana}")
            f.write(f"{result}\n")


style.use("fivethirtyeight")
print(banana)
fig = plt.figure(figsize=(8,12))
ax1 = fig.add_subplot(1, 1, 1)
ax1.set_ylim([0, 750])
print(banana)
xdata = deque([i for i in range(100)], maxlen=100)
ydata = deque([0.0] * 100, maxlen=100)
(line,) = ax1.plot(xdata, ydata, lw=2)

plt.yticks(np.arange(0, 750, 25))

def animate(i, ydata):
    ydata.append(result)
    line.set_ydata(ydata)
    return (line,)


graph_t = threading.Thread(target=conn, daemon=True)
graph_t.start()
ani = animation.FuncAnimation(fig, animate, fargs=(ydata,), interval=100, blit=True)
plt.show()


# Close handle
# ljm.close(handle)