import serial
import time
import paho.mqtt.client as paho
import matplotlib.pyplot as plt
import numpy as np
mqttc = paho.Client()

# Settings for connection
host = "localhost"
topic= "velocity"
port = 1883

# XBee setting
serdev = '/dev/ttyUSB0'
s = serial.Serial(serdev, baudrate=9600)

s.write("+++".encode())
char = s.read(2)
print("Enter AT mode.")
print(char.decode())

s.write("ATMY 0x205\r\n".encode())
char = s.read(3)
print("Set MY 0x205.")
print(char.decode())

s.write("ATDL 0x305\r\n".encode())
char = s.read(3)
print("Set DL 0x305.")
print(char.decode())

s.write("ATID 0x1\r\n".encode())
char = s.read(3)
print("Set PAN ID 0x1.")
print(char.decode())

s.write("ATWR\r\n".encode())
char = s.read(3)
print("Write config.")
print(char.decode())

s.write("ATMY\r\n".encode())
char = s.read(4)
print("MY :")
print(char.decode())

s.write("ATDL\r\n".encode())
char = s.read(4)
print("DL : ")
print(char.decode())

s.write("ATCN\r\n".encode())
char = s.read(3)
print("Exit AT mode.")
print(char.decode())

print("start sending RPC")
all_string = []

s.write("/GetData/run\r".encode())
for i in range(1000):
    indata = s.readline()
    all_string.append(indata)
print(indata)
        
# Callbacks
def on_connect(self, mosq, obj, rc):
    print("Connected rc: " + str(rc))

def on_message(mosq, obj, msg):
    print("[Received] Topic: " + msg.topic + ", Message: " + str(msg.payload) + "\n");

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

for s in all_string:
    mesg = s
    mqttc.publish(topic, mesg)
    print(mesg)
    time.sleep(0.1)

s.close()