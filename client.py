import sys
import json
import numpy as np
from datetime import datetime
import paho.mqtt.client as mqtt             # pip install paho-mqtt

# Live charts
import matplotlib.pyplot as plt             # pip install matplotlib OR pacman -S  pacman -S mingw-w64-ucrt-x86_64-python-matplotlib
import matplotlib.animation as animation

sys.path.append("./AsconPy/build/")
import AsconPy

######################
### Configuration  ###
######################

PORT = 1883
ADDRESS = "127.0.0.1"
KEY = np.array([0x0e, 0xdc, 0x97, 0xa7, 0x95, 0xc1, 0x08, 0x65, 0xbf, 0x7d, 0xac, 0x1c, 0xc8, 0x53, 0x23, 0x48], np.uint8)

###################
### Live charts ###
###################

class AnimatedChart:
    def __init__(self, name, title_x, title_y, datatype):
        self._fig = plt.figure()
        self._fig.canvas.manager.set_window_title(f"{name} - Chart")
        self._plt = self._fig.add_subplot(1,1,1)
        self._plt.set_title(name)
        self._plt.set_xlabel(title_x)
        self._plt.set_ylabel(title_y)
        self._datatype = datatype

        def animate(i):
            if (datatype not in data): return
            
            self._plt.clear()
            self._plt.set_title(name)
            self._plt.set_xlabel(title_x)
            self._plt.set_ylabel(title_y)

            for client_id in data[datatype]:
                client_data = data[datatype][client_id]
                self._plt.plot(client_data['datetime'], client_data['val'], label=client_id)

            self._plt.legend()

        self._animation = animation.FuncAnimation(self._fig, animate, interval=1000, cache_frame_data=False)

        self._fig.show()

#########################
### MQTT client setup ###
#########################

ascon = AsconPy.Ascon(KEY)

data = {}

charts = {
    'temperature': AnimatedChart("Temperature", "datetime", "°C", "temperature"),
    'humidity': AnimatedChart("Humidity", "datetime", "humidity", "humidity")
}

def print_byte_arr(arr):
    print(f"[{','.join('0x{:02x}'.format(x) for x in arr)}]")

def handle_data_msg(type_str, payload):
    # Update the data reading when receiving a value
    try:
        # Decrypt
        payload = np.frombuffer(payload, dtype=np.uint8)
        type_arr = np.frombuffer(type_str.encode('ascii'), dtype=np.uint8)
        
        payload = ascon.decrypt(payload, type_arr)
        if (len(payload) == 0): 
            print("Invalid playload")
            return
            # Parse

        # Remove last zero byte
        if (payload[-1] == 0): payload = payload[:-1]
        payload_str = payload.tobytes().decode("UTF-8")
        payload = json.loads(payload_str)
    except:
        print(f"Invalid message received: {str(payload)}")
        if (payload_str):
            print(f"\t=> {payload_str}")
        return

    if (type_str not in data): data[type_str] = {}
    
    client_id = payload['id']
    value     = float(payload['val'])

    now = datetime.now()


    if (client_id not in data[type_str]):
        data[type_str][client_id] = { 'datetime': np.array([]), 'val': np.array([]) }

    data[type_str][client_id]['datetime'] = np.append(data[type_str][client_id]['datetime'], now)
    data[type_str][client_id]['val'] = np.append(data[type_str][client_id]['val'], value)

def on_message(client, userdata, msg):
    match msg.topic:
        case "temperature":
            handle_data_msg("temperature", msg.payload)
        case "humidity":
            handle_data_msg("humidity", msg.payload)

client = mqtt.Client()
client.on_message = on_message
client.connect(ADDRESS, PORT, 60)

client.loop_start()

client.subscribe("temperature")
client.subscribe("humidity")

input("Push any button to close")
print("Stopping MQTT client...")
client.loop_stop()
print("MQTT client stopped successfully")