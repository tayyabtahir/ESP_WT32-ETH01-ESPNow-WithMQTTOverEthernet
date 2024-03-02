# WT32-ETH01 with ESPNow and MQTT Client.

## Description

This project implements a sample code to get data over ESPNOW and send this data to an MQTT broker.

## Features

- Utilizes Arduino enviroment for easy tinkering
- Integrates ESPnow features
- Sends data to an MQTT broker for [any] processing

## Usage

1. Clone the repository.
2. Add your MQTT client credentials to MQTT_BROKER_PATH, MQTT_BROKER_PORT, MQTT_PUBLISH_TOPIC and MQTT_CLIENT_ID.
3. Upload the code to your device.
5. Configure the MQTT broker settings in the code.
6. Run the code and monitor Data via the MQTT broker.

## Requirements

- WT32-ETH01
- MQTT Client/Brocker[https://www.hivemq.com/demos/websocket-client/]

## License

This project is licensed under the [MIT License](LICENSE).
