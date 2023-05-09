
import os
import time
import paho.mqtt.client as mqtt
import serial
import json

time.sleep(0.5)

THINGSBOARD_HOST = 'thingsboard.cs.cf.ac.uk'
ACCESS_TOKEN = 'sJrrcwDBLO4nIuE75yah'  # 
SERIAL_PORT = 'COM6'
BAUD_RATE = 9600


# Create a MQTT client
client = mqtt.Client()

# Set the access token as username
client.username_pw_set(ACCESS_TOKEN)

# Connect to ThingsBoard
client.connect(THINGSBOARD_HOST, 1883, 60)

# Start the MQTT client
client.loop_start()


ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
try:
    while True:
        # Read a line from the serial port
        line = ser.readline().decode('utf-8').strip()
        json_string = line.replace("'", "\"")

        try:
            data = json.loads(json_string)
        except json.JSONDecodeError:
            print("Error: Invalid or empty JSON string received.")
            continue
        if data["fullness"] > 600:
            data["fullness"] = float(0)
        else:
            data["fullness"] = 100 -((data["fullness"] -20) / 880 *100)

        if data["fullness"] > 100:
            data["fullness"] = float(100)

        
        payload = json.dumps(data)


        #Publish data to the 'v1/devices/me/telemetry' topic
        client.publish('v1/devices/me/telemetry', payload, qos=1)
        print(f'Sent data: {payload}')

except KeyboardInterrupt:
    # Close the serial connection and MQTT client on keyboard interrupt
    # ser.close()
    # client.disconnect()
    print("Disconnected")






