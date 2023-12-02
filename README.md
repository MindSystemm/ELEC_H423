# ELEC_H423
IoT Project for ELEC_H423 course

# Use

## Broker
Install and start mosquitto: https://mosquitto.org/download/
A script is provided to make starting it easier: ```start_broker.bat```

## Client
```client.py``` is a terminal application used to communicate with the sensors.
It can display the most recent temperatures, and ask the microcontrollers to display a message on their LCDs.

## Sensors
### Production
To start the sensors, change the necesarry parameters such as ids and keys of ```ESP.ino```, and upload it to the microcontrollers.

### Dummy
To test the client and MQTT broker a dummy sensor script is included: ```dummy.py```.
When executed, it will behave like a sensor.

# Installed boards
- esp32

# Installed libraries
## IDE
- DHT sensor library
- PubSubClient
## Manual
- Crypto (to use this with and ESP32 rename ```crypto.h``` in ```%appdata%/../local/Arduino15/packages/esp32/hardware/esp32/2.0.11/tools/sdk/esp32/include/bt/esp_ble_mesh/mesh_core```)
- CryptoLW

# Secret values
The file ```Config.h``` contains secret values hand has been ignored after committing initial values with:
```bash
git update-index --skip-worktree ESP/Config.h
```
It can be re-added to the worktree using:
```bash
git update-index --no-skip-worktree ESP/Config.h
```

# TODO
[] Encryption:
- TLS
- ASCON
- RSA
- DH with AES
