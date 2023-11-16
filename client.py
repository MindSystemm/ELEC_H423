import re
import socket
import json
import numpy as np
from datetime import datetime
import paho.mqtt.client as mqtt

# Live charts
import matplotlib.pyplot as plt
import matplotlib.animation as animation

######################
### Configuration  ###
######################

PORT = 1883
ADDRESS = "127.0.0.1"

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

data = {}

charts = {
    'temp': AnimatedChart("Temperature", "datetime", "°C", "temp"),
    'humi': AnimatedChart("Humidity", "datetime", "humidity", "humi")
}

def handle_data_msg(type_str, payload):
    # Update the temperature reading when receiving a value
    try:
        payload = json.loads(payload.decode("UTF-8"))
    except:
        print(f"Invalid message received: {str(payload)}")

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
            handle_data_msg("temp", msg.payload)
        case "humidity":
            handle_data_msg("humi", msg.payload)

client = mqtt.Client()
client.on_message = on_message
client.connect(ADDRESS, PORT, 60)

client.loop_start()

client.subscribe("temperature")
client.subscribe("humidity")

#####################
### Main behavior ###
#####################    

def display_on_clients(msg):
    client.publish("display", msg)

def show_data(name_str, type_str, unit):
    if (type_str not in data): data[type_str] = {}

    print(f"{name_str}:")
    for client_id in data[type_str]:
        # Get the last not NaN value
        datetime = data[type_str][client_id]['datetime'][-1]
        val      = data[type_str][client_id]['val'][-1]
        
        print(f"\t{client_id}: {val}{unit} (last update: {datetime})")

def show_broker_info():
    ip = ADDRESS
    if (ip == "localhost" or ip.startswith("127.")):
        # Local => print own IP
        local_hostname = socket.gethostname()
        ip_addresses = socket.gethostbyname_ex(local_hostname)[2]
        filtered_ips = [ip for ip in ip_addresses if not ip.startswith("127.")]
        ip = ', '.join(filtered_ips)

    print(f"Broker info:\n\taddress: {ip}\n\tport: {PORT}")

####################
### Command Line ###
####################

def start_terminal():
    cmd = []
    while(len(cmd) == 0 or not cmd[0] == "stop"):
        regex = r"((?:\"[^\"\\]*(?:\\[\S\s][^\"\\]*)*\"|'[^'\\]*(?:\\[\S\s][^'\\]*)*'|\/[^\/\\]*(?:\\[\S\s][^\/\\]*)*\/[gimy]*(?=\s|$)|(?:\\\s|\S))+)(?=\s|$)"

        print("Broker> ", end="")
        input1 = str(input())
        cmd = [ s for s in re.split(regex, input1) if not s == '' and not s == ' ' ]

        # Clean quotes around strings
        for i, s in enumerate(cmd):
            if (len(s) >= 2):
                if ((s[:1] == '"' and s[-1:] == '"') or (s[:1] == "'" and s[-1:] == "'")):
                    cmd[i] = s[1:-1]

        def unexpected_args(expectedAmount, correctUsage):
            print(f"Unexpected arguments: expected {expectedAmount}, got {len(cmd) - 1}\nCorrect usage: ${correctUsage}")

        # Handle commands
        match cmd[0].lower():
            case 'help':
                if (len(cmd) != 1):
                    unexpected_args(0, "help")
                    continue
                print("Commands:\n\thelp: shows this screen\n\tdisplay [MESSAGE]: displays a message on the client LCDs\n\ttemp: displays the current client temperatures\n\thumidty: displays the current client humidities\n\tbroker: shows the broker information\n\texit: stops the server and exits")
            case 'display':
                if (len(cmd) != 2):
                    unexpected_args(1, "display [MESSAGE]")
                    continue
                display_on_clients(cmd[1])
            case 'temp':
                if (len(cmd) != 1):
                    unexpected_args(0, "temp")
                    continue
                show_data("Temperature", "temp", "°C")
            case 'humidity':
                if (len(cmd) != 1):
                    unexpected_args(0, "humidity")
                    continue
                show_data("Humidity", "humi", "")
            case 'broker':
                if (len(cmd) != 1):
                    unexpected_args(0, "broker")
                    continue
                show_broker_info()
            case 'exit':
                if (len(cmd) != 1):
                    unexpected_args(0, "exit")
                    continue
                print("Stopping MQTT client...")
                client.loop_stop()
                print("MQTT client stopped successfully")
                exit(0)
            case _:
                print(f"Unknown command: {cmd[0]}, use 'help' for a list of supported commands")

        # Add extra line after command
        print()

start_terminal()