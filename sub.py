import paho.mqtt.client as paho
import matplotlib.pyplot as plt
import numpy as np
import serial
import time
mqttc = paho.Client()

# Settings for connection
host = "localhost"
topic= "Mbed"
port = 1883

velocity_list = []
count = 0

Fs = 2.0
Ts = 1.0 / Fs
time_vec = np.arange(0, 20, Ts)
velocity_vec = np.arange(0, 20, Ts)

# Callbacks
def on_connect(self, mosq, obj, rc):
    print("Connected rc: " + str(rc))

def on_message(mosq, obj, msg):
    global count
    print('count = ', end = '')
    print(count)
    print("[Received] Topic: " + msg.topic + ", Message: " + str(msg.payload) + "\n")

    velocity_list.append(int(msg.payload))
    count += 1
    
def on_subscribe(mosq, obj, mid, granted_qos):
    print("Subscribed OK")

def on_unsubscribe(mosq, obj, mid, granted_qos):
    print("Unsubscribed OK")

# Set callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_subscribe = on_subscribe
mqttc.on_unsubscribe = on_unsubscribe

# Connect and subscribe
print("Connecting to " + host + "/" + topic)
mqttc.connect(host, port=1883, keepalive=60)
mqttc.subscribe(topic, 0)
mqttc.loop_start()