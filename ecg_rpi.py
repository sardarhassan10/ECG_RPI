# IoMT-Based ECG Monitoring using an ESP8266 and a Raspberry Pi
# Embedded System Design End-Of-Semester Project, SEECS, NUST

# By: Zain Amir Zaman, Sardar Hassan Arfan Khan,
#     Abdul Rehman Qamar Aftab

import numpy as np
import matplotlib.pyplot as plt
import time
from scipy.signal import find_peaks
import neurokit2 as nk
import paho.mqtt.client as paho

hr_window_length = 10  # Length of ECG buffer
update_interval = 2  # Time interval between updates of HR
update_timestamp = hr_window_length - update_interval
sampling_time = 60
broker_addr = "your address"  # Set to own local IP address
ecg_buffer = []
time_buffer = []
hr_buffer = []
data_i = 0
buffer_filled = False

plt.ion()  # Enable interactive mode to allow plots to update


# Actions to perform once connected to broker

def on_connect(client, userdata, flags, rc):
    print("Connected with RPi Broker with result code " + str(rc))
    client.subscribe("ECG/data")
    print("Waiting for data from topic ECG/data...")


# Insert an element to the last position of an array and remove
# the first element

def push(arr, val):
    x = np.append(arr, val)
    return x[1:]


# Find the heart rate from a given segment of the ECG signal

def find_hr(signal, t):
    _, r_peaks = nk.ecg_peaks(signal, sampling_rate=500)
    r_peaks = r_peaks['ECG_R_Peaks']

    num_beats = len(r_peaks)
    window_len = (t[r_peaks[-1]] - t[r_peaks[0]])
    hr = (num_beats-1) * 60 / window_len

    # Update ECG plot

    plt.clf()
    plt.plot(t, signal)
    plt.plot(t[r_peaks], signal[r_peaks], "x")
    plt.title("Electrocardiogram (ECG), Heart Rate: %.1f bpm" % hr)
    plt.xlabel("Time (seconds)")
    plt.ylabel("Amplitude (arbitrary)")
    plt.pause(0.0001)
    plt.show()

    return hr


# Actions to perform upon receiving data from broker

def on_message(client, userdata, message):
    global ecg_buffer
    global time_buffer
    global hr_buffer
    global data_i
    global hr_window_length
    global update_interval
    global update_timestamp
    global buffer_filled

    msg = message.payload.decode("utf-8")

    # Order of messages: t, e, t, e, t, e, ...
    # where t = timestamp and e = ECG sample

    if data_i % 2 == 1:  # Grab timestamp
        time_val = int(msg) / 1000

        if time_val < hr_window_length:  # If buffers are not full...
            time_buffer = np.append(time_buffer, time_val)
        else:
            if not buffer_filled:
                buffer_filled = True

            time_buffer = push(time_buffer, time_val)

            # Find HR after a periodic update interval

            if time_val > update_timestamp + update_interval:
                hr = find_hr(ecg_buffer[0:-1], time_buffer)
                print(hr)
                hr_buffer = np.append(hr_buffer, hr)
                update_timestamp = time_val

    else:  # Grab ECG sample
        if not buffer_filled:
            ecg_buffer = np.append(ecg_buffer, float(msg))
        else:
            ecg_buffer = push(ecg_buffer, float(msg))

    data_i += 1

    # Stop after sampling period is over

    if time_buffer[-1] >= sampling_time:
        client.disconnect()
        client.loop_stop()


# Create subscriber / client

client = paho.Client("RPI-Subscriber")
client.on_connect = on_connect
client.on_message = on_message

# Connect to broker

print("Connecting to RPi MQTT Broker: ", broker_addr)
client.connect(broker_addr)
client.loop_forever()
