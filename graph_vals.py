from labjack import ljm
from time import sleep
import time
from matplotlib import pyplot


sensor_name = input("Enter pressure sensor name: ").upper()

handle = ljm.openS("ANY", "ANY", "ANY")

info = ljm.getHandleInfo(handle)


print(
    "Opened a LabJack with Device type: %i, Connection type: %i,\n"
    "Serial number: %i, IP address: %s, Port: %i,\nMax bytes per MB: %i"
    % (info[0], info[1], info[2], ljm.numberToIP(info[3]), info[4], info[5])
)

# Setup and call eReadName to read from AIN0 on the LabJack.
name = "AIN2"
# result = ljm.eReadName(handle, name)
# print("\n%s reading : %f C" % (name, result))
names = ["AIN2_NEGATIVE_CH", "AIN2_RANGE"]
aValues = [3.0, 0.1]

ljm.eWriteNames(handle, len(names), names, aValues)
with open(f"logs/{sensor_name}_{int(time.time())}.txt", "w+") as f:
    maxbanana = -1000000
    while True:
        sleep(0.5)
        v = ljm.eReadName(handle, name)
        result = v * 25000
        maxbanana = max(maxbanana, result)
        print("\n%s reading : %f" % (name, result))
        print(f"max is {maxbanana}")
        f.write(f"{result}\n")

# Close handle
ljm.close(handle)
