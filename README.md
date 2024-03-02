# WT32-ETH01_ESPNowSample

Developing samples here on WT32-ETH01 to be used in broader applications


## Roadmap

- Additional browser support

- Add more integrations


## How To program WT32-ETH01
Connect your USB->TTL Flasher with your WT32-ETH01 (yes, RX and TX is crossed!!!), here are the connection details

* GND ------> GND
* TX  ------> RX0 (not RXD)
* RX  ------> TX0 (not RXD)
* 5V  ------> 5V
* Then only on the WT32-ETH01: Connect the Pin “IO0” (next to RX0) and “GND” (next to IO0) together.

### Please follow the link here for furthur details
https://wled.discourse.group/t/wt32-eth01-lan-wifi-flash-tutorial/2786




## Application Details
A brief description on the applications involved

### WT32-ETH01-ESPNow-WithMQTTOverEthernet.ino
This sample code drives esp wifi into esnow receiver and then receives the data from other espnow transmittors. Afterwards sends this data to a Mqtt broker over ethernet.

### WT32-ETH01_HTTPS_Over_Ethernet_With_ESPNow.ino
This sample code drives esp wifi into esnow receiver and then receives the data from other espnow transmittors. Afterwards sends this data to a HTTPS server over ethernet.

## Required Setup & Libraries

In order to compile, Please get the **Arduino IDE** then proceed:

* **Make sure to add following libraries**
    * ArduinoMqttClient 
    * ArduinoQueue
    * WiFiClientSecure
    * EthernetUdp
    * TimeLib

* **Select the Board as "WT32-ETH01 Ethernet Module".**
