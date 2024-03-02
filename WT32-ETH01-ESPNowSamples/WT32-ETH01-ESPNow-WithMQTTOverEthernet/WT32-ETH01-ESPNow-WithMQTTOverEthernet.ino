//==============================================================================
//  FILE INFORMATION
//==============================================================================
//
//  Source:        
//
//  Project:       
//
//  Author:        
//
//  Date:          
//
//  Revision:      1.0
//
//==============================================================================
//  FILE DESCRIPTION
//==============================================================================
//
//! \file
//! 
//! 
//
//==============================================================================
//  REVISION HISTORY
//==============================================================================
//  Revision: 1.0  
//      
//
//==============================================================================
//  INCLUDES
//==============================================================================
#include <ArduinoMqttClient.h>
#include <ETH.h>
#include <ArduinoQueue.h>
#include <esp_now.h>
#include <WiFi.h>

//==============================================================================
//	CONSTANTS, TYPEDEFS AND MACROS 
//==============================================================================


//==============================================================================
//	LOCAL DATA STRUCTURE DEFINITION
//==============================================================================
#define CHANNEL 1
#define MQTTTopicLength		128
#define MQTTDataLength		256
#define MQTTQueueMaxElem  10
#define MACIDSTRINGLENGTH 18

struct QueueElem_t
{
	char Topic[MQTTTopicLength];
  char MacID[MACIDSTRINGLENGTH];
	char Data[MQTTDataLength];
};

//==============================================================================
//	GLOBAL DATA DECLARATIONS
//==============================================================================
WiFiClient ethClient;
MqttClient mqttClient(ethClient);

static bool eth_connected = false;
static bool isStartupComplete = false;

ArduinoQueue<QueueElem_t> DataQueue(MQTTQueueMaxElem);
// To be filled by the user, please visit: https://www.hivemq.com/demos/websocket-client/
const char MQTT_BROKER_PATH[] = "broker.hivemq.com";
int        MQTT_BROKER_PORT   = 1883;
const char MQTT_PUBLISH_TOPIC[]  = "WT32-ETH01/espNowEndpoint";
const char MQTT_CLIENT_ID[] = "clientId-tPbkEIA7IS";

//==============================================================================
//	LOCAL FUNCTION PROTOTYPES
//==============================================================================
void WiFiEvent(WiFiEvent_t event);
void PublishDataToMQTTBroker(char *topic, char *macID, char *data);
bool InsertElemInQueue(char* MQTTTopic, char* MacID, char *data, int data_len);
void InitESPNow();
void configDeviceAP();
void OnESPNowDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);

//==============================================================================
//	LOCAL AND GLOBAL FUNCTIONS IMPLEMENTATION
//==============================================================================

//==============================================================================
//
//  void loop()
//
//!  This function is the Main Activity thread
//
//==============================================================================
void loop()
{
  /*
  ##############################################################
  ##############################################################
                Uncomment Test Code for Dummy MQTT Events
  ##############################################################
  ##############################################################
  */
  // static int DataCount = '0' + 0;
  // char Buffer[4];
  // char macStr[18] = "0C:B8:15:4A:E0:80";
  // memset(Buffer, 0, 4);
  // memcpy(Buffer, &DataCount, 4);
  // InsertElemInQueue((char*)MQTT_PUBLISH_TOPIC, macStr, (char*)Buffer, 4);
  // DataCount++;


  if (eth_connected) 
  {
    if (!mqttClient.connected()) 
    {
      ReconnectMQTTClient();
    }
    
    // call poll() regularly to allow the library to send MQTT keep alives which avoids being disconnected by the broker
    if (DataQueue.isEmpty ())
    {
      mqttClient.poll();
    }
    else
    {
      while (!DataQueue.isEmpty ())
      {
        QueueElem_t elem;
        elem = DataQueue.dequeue();
        PublishDataToMQTTBroker(elem.Topic, elem.MacID, elem.Data);
      }
    }
  }

  delay(10000);
}

//==============================================================================
//
//  void setup()
//
//!  This function handels all the initialization Activities
//
//==============================================================================
void setup()
{
  
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial); // wait for serial port to connect. Needed for native USB port only


  /*
  ##############################################################
  ##############################################################
                CONNECT TO NETWORK VIA ETHERNET 
  ##############################################################
  ##############################################################
  */

  Serial.println("Connecting to LAN");
  Serial.println();
  //Ethernet Initialization
  WiFi.onEvent(WiFiEvent);
  while (ETH.begin() != true)
  {
    // failed, retry
    Serial.print(".");
    delay(1000);    
  }
    //WiFiClient client;
  if (!ethClient.connect(MQTT_BROKER_PATH, MQTT_BROKER_PORT)) {
    Serial.println("connection failed");
    return;
  }
  Serial.println("You're connected to the network");
  Serial.println();


  /*
  ##############################################################
  ##############################################################
                CONFIGURE WIFI IN ESPNOW MODE
  ##############################################################
  ##############################################################
  */

  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnESPNowDataRecv);

  /*
  ##############################################################
  ##############################################################
                CONNECT TO MQTT BROKER
  ##############################################################
  ##############################################################
  */
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(MQTT_BROKER_PATH);

  mqttClient.setId(MQTT_CLIENT_ID);

  if (!mqttClient.connect(MQTT_BROKER_PATH, MQTT_BROKER_PORT)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  // Allow the hardware to sort itself out
  delay(5000);

  isStartupComplete = true;
}

//==============================================================================
//
//  void WiFiEvent(WiFiEvent_t event)
//
//!  This function handels Ethernet Event Call Backs
//
//==============================================================================
void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

//==============================================================================
//
//  void PublishDataToMQTTBroker(char *topic, char *macID, char *data)
//
//!  This function handels MQTT data publishing to the MQTT broker
//
//==============================================================================
void PublishDataToMQTTBroker(char *topic, char *macID, char *data)
{
    Serial.println("Sending Data To MQTT Broker");
    Serial.print("Topic: "); Serial.println(topic);
    Serial.print("MACID: "); Serial.println(macID);
    Serial.print("Data: ");  Serial.println(data);
    Serial.println();

    //send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print(macID); mqttClient.print(" ::Data:: "); mqttClient.print(data);
    mqttClient.endMessage();
}

//==============================================================================
//
//  bool InsertElemInQueue(char* MQTTTopic, char* MacID, char *data, int data_len)
//
//!  This function insert an elem into publish Queue
//
//==============================================================================
bool InsertElemInQueue(char* MQTTTopic, char* MacID, char *data, int data_len)
{
  QueueElem_t QueueItem;
  memset(&QueueItem, 0x00, sizeof(QueueItem));

  Serial.println("Putting Element In Queue.......");
  Serial.println();
  memcpy(QueueItem.Topic, MQTTTopic, strlen(MQTTTopic));
  memcpy(QueueItem.MacID, MacID, strlen(MacID));
  memcpy(QueueItem.Data, data, data_len);
  
  DataQueue.enqueue(QueueItem);

  Serial.println("Successfully");
  Serial.print("Topic: "); Serial.println(QueueItem.Topic);
  Serial.print("MacID: "); Serial.println(QueueItem.MacID);
  Serial.print("Data: "); Serial.println(QueueItem.Data);
  Serial.println();

  return true;
}

//==============================================================================
//
//  void reconnect()
//
//!  This function reconnects MQTT client
//
//==============================================================================
void ReconnectMQTTClient() 
{
  Serial.print("Attempting MQTT ReConnection...");

  // Loop until we're reconnected
  while (!mqttClient.connected()) 
  {  
    mqttClient.setId(MQTT_CLIENT_ID);

    if (!mqttClient.connect(MQTT_BROKER_PATH, MQTT_BROKER_PORT)) 
    {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(mqttClient.connectError());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}

//==============================================================================
//
//  void InitESPNow() 
//
//!  This function Init ESP Now with fallback
//
//==============================================================================
void InitESPNow() 
{
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) 
  {
    Serial.println("ESPNow Init Success");
  }
  else 
  {
    Serial.println("ESPNow Init Failed");
    Serial.println("***********************************************");    
    Serial.println("***********     REBOOTING       ***************");
    Serial.println("***********************************************");
    Serial.println();
    
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

//==============================================================================
//
//  void configDeviceAP() 
//
//!  This function config Device in AP mode
//
//==============================================================================
void configDeviceAP() 
{
  String Prefix = "Slave:";
  String Mac = WiFi.macAddress();
  String SSID = Prefix + Mac;
  String Password = "123456789";
  bool result = WiFi.softAP(SSID.c_str(), Password.c_str(), CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
  }
}

//==============================================================================
//
//  void OnESPNowDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) 
//
//!  This function manages callback when data is recv from Master
//
//==============================================================================
void OnESPNowDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) 
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.print("Last Packet Recv Data: "); Serial.println(*data);
  Serial.println("");

  InsertElemInQueue((char*)MQTT_PUBLISH_TOPIC, macStr, (char*)data, data_len);
}
