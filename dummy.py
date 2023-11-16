import random
import time
import paho.mqtt.client as mqtt

######################
### Configuration  ###
######################

PORT = 1883
ADDRESS = "127.0.0.1"

ID = "Dummy sensor 1"

#########################
### MQTT client setup ###
#########################

def on_message(client, userdata, msg):
    print(f"{msg.topic} {msg.payload.decode('UTF-8')}")

client = mqtt.Client()
client.on_message = on_message
client.connect(ADDRESS, PORT, 60)

client.subscribe("display")

client.loop_start()

while (True):
    client.publish("temperature", f"{{ \"val\": {random.randint(0, 30)}, \"id\": \"{ID}\" }}")
    client.publish("humidity", f"{{ \"val\": {random.randint(0, 100)}, \"id\": \"{ID}\" }}")
    time.sleep(2)