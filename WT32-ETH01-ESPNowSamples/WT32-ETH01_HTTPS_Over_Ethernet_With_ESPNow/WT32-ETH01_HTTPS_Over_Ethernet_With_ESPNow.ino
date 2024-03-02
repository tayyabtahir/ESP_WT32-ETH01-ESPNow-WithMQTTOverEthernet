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
#include <ETH.h>
#include <ArduinoQueue.h>
#include <esp_now.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "time.h"

//==============================================================================
//	CONSTANTS, TYPEDEFS AND MACROS
//==============================================================================


//==============================================================================
//	LOCAL DATA STRUCTURE DEFINITION
//==============================================================================
#define FW_MAJOR_VER  1
#define FW_MINOR_VER  0

#define CHANNEL 1
#define MACIDSTRINGLENGTH 18
#define SNIDSTRINGLENGTH 6
#define BUTTONDATASTRINGLENGTH 4
#define HTTPREQBUFFLENGTH 256
#define QUEUEMAXLENGTH 10

#define BIT_SIZE_BYTE 8
#define BYTE_SIZE_SHORT 2
#define BYTE_SIZE_INT 4

#define ESP_MESSAGE_SIZE (SNIDSTRINGLENGTH + BUTTONDATASTRINGLENGTH)

struct QueueElem_t {
  unsigned long recvTimeStampSeconds;
  unsigned int recvTimeStampMillisChange;
  char MacID[MACIDSTRINGLENGTH];
  char SnID[SNIDSTRINGLENGTH];
  uint32_t ButtonData;
};

//==============================================================================
//	GLOBAL DATA DECLARATIONS
//==============================================================================
WiFiClientSecure ethClient;

static bool eth_connected = false;
static bool isStartupComplete = false;

ArduinoQueue<QueueElem_t> DataQueue(QUEUEMAXLENGTH);

/*
    ==============================================
          NTP Server Communication variables
    ==============================================
*/
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;
unsigned long localTickAtTimeUpdate = 0;

/*
    ==============================================
          Sever communication variables
    ==============================================
*/
// Please fill in the details of your desired http server
HTTPClient https;
String HTTP_METHOD_GET = "GET";
String HTTP_METHOD_POST = "POST";
String HOST_NAME = "https://httpbin.org/";
String HOST_API_ENDPOINT = "https://httpbin.org//entry";
int HTTP_PORT = 443;

const char *rootCACertificate =
  "-----BEGIN CERTIFICATE-----\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
"TestCertificateTestCertificateTestCertificateTestCertificate0001\n"
  "-----END CERTIFICATE-----\n";


//==============================================================================
//	LOCAL FUNCTION PROTOTYPES
//==============================================================================
void WiFiEvent(WiFiEvent_t event);
void InitESPNow();
void ConfigDeviceAP();
void OnESPNowDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void CovertUnsignedShortToBinaryMap(uint16_t num, char *OutputBinArray);
bool InsertElemInQueue(char *MacID, char *data, int data_len);
void PrintCurrentTime();
unsigned long GetCurrentEpoochTimeStamp(void);


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
void loop() {
  static int counterToPrint = 0;

  if (eth_connected) {
    while (!DataQueue.isEmpty()) {
      QueueElem_t elem;
      elem = DataQueue.dequeue();
      PostDataToServer(elem.MacID, elem.SnID, elem.ButtonData, elem.recvTimeStampSeconds, elem.recvTimeStampMillisChange);
    }
  }

  // // Test Code- uncomment for dummy events
  // static char dummyData[10]= "ABCDEF001";
  // if (counterToPrint++ > 1000000) {
  //   Serial.println("Looped 10000");
  //   counterToPrint = 0;
  //   InsertElemInQueue("12345678901", (char*)&dummyData, ESP_MESSAGE_SIZE);
  // }
}

//==============================================================================
//
//  void setup()
//
//!  This function handels all the initialization Activities
//
//==============================================================================
void setup() {

  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial)
    ;  // wait for serial port to connect. Needed for native USB port only


  Serial.println("***********************************************");
  Serial.printf ("***************    FW: %.*d.%.*d     **************\n", 2,FW_MAJOR_VER,2,FW_MINOR_VER);
  Serial.println("***********************************************");
  Serial.println();
  /*
  ##############################################################
  ##############################################################
                INITIALIZE ETHERNET SHIELD
  ##############################################################
  ##############################################################
  */
  Serial.println("[STARTUP] Connecting to LAN");
  Serial.println();
  //Ethernet Initialization
  WiFi.onEvent(WiFiEvent);
  while (ETH.begin() != true) {
    // failed, retry
    Serial.print(".");
    delay(1000);
  }
  // DHCP assignment successful, display ethernet shield IP to serial
  // [part of Ethernet Library]
  Serial.print("[STARTUP] Ethernet Shield IP (DHCP): ");
  Serial.println(ETH.localIP());

  /*
  ##############################################################
  ##############################################################
        SETUP NTP SERVER COMMUNICATION FOR REALTIME CLOCK
  ##############################################################
  ##############################################################
  */

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  localTickAtTimeUpdate = millis();
  PrintCurrentTime();


  /*
  ##############################################################
  ##############################################################
        SETUP FOR SERVER COMMUNICATION 
  ##############################################################
  ##############################################################
  */
  Serial.println("[STARTUP] Setting Up Certificate for HTTPS communication with the Server");
  Serial.println();
  ethClient.setCACert(rootCACertificate);


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
  ConfigDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("[STARTUP] AP MAC: ");
  Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnESPNowDataRecv);


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
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("[ETH EVENT] ETH Started");
      //set eth hostname here
      ETH.setHostname("[ETH EVENT] esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("[ETH EVENT] ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("[ETH EVENT] ETH MAC: ");
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
      Serial.println("[ETH EVENT] ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("[ETH EVENT] ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

//==============================================================================
//
//  void PostDataToServer(char *macID, char *snID, uint32_t ButtonData, unsigned long TimeStamp, unsigned int TimeStampMilliSecondsChange)
//
//!  This function handels Https data publishing to the server
//
//==============================================================================
void PostDataToServer(char *macID, char *snID, uint32_t ButtonData, unsigned long TimeStamp, unsigned int TimeStampMilliSecondsChange) {

  int httpResponseCode = -1;
  char httpPostRequest[HTTPREQBUFFLENGTH] = { 0 };
  int httpsBodyLength = 0;
  // char ShortIntToBinaryFormat[BYTE_SIZE_SHORT * BIT_SIZE_BYTE] = { 0 };

  // Convert Button data to binary map
  // CovertUnsignedShortToBinaryMap(ButtonData, ShortIntToBinaryFormat);

  https.begin(ethClient, HOST_API_ENDPOINT);
  https.addHeader("Content-Type", "application/json");
  //"{\"data\": [{\"buttons\": \"0000000000010000\",\"ts\": 100}]}"
  /*
  httpsBodyLength = snprintf(httpPostRequest, HTTPREQBUFFLENGTH,
                             "{\"data\": [{\"buttons\": \"%.*s\",\"ts\": %lu}]}",
                             ShortIntToBinaryFormat,BYTE_SIZE_SHORT * BIT_SIZE_BYTE, TimeStamp);
                             */

  /*
  httpsBodyLength = snprintf(httpPostRequest, HTTPREQBUFFLENGTH,
                             "{\"data\": [{\"buttons\": \"%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d\",\"ts\": %lu%.*d}]}",
                             ShortIntToBinaryFormat[15], ShortIntToBinaryFormat[14], ShortIntToBinaryFormat[13], ShortIntToBinaryFormat[12], ShortIntToBinaryFormat[11], ShortIntToBinaryFormat[10], ShortIntToBinaryFormat[9], ShortIntToBinaryFormat[8],
                             ShortIntToBinaryFormat[7], ShortIntToBinaryFormat[6], ShortIntToBinaryFormat[5], ShortIntToBinaryFormat[4], ShortIntToBinaryFormat[3], ShortIntToBinaryFormat[2], ShortIntToBinaryFormat[1], ShortIntToBinaryFormat[0],
                             TimeStamp, 3, TimeStampMilliSecondsChange);
                             */
  /*
  httpsBodyLength = snprintf(httpPostRequest, HTTPREQBUFFLENGTH,
                            "{\"data\": [{\"sn\": \"%.*s\",\"buttons\": \"%d\",\"ts\": %lu%.*d}]}",
                            6, snID, ButtonData, TimeStamp, 3, TimeStampMilliSecondsChange);
  */

  httpsBodyLength = snprintf(httpPostRequest, HTTPREQBUFFLENGTH,
                            "{\"data\": [{\"sn\": \"%02x%02x%02x%02x%02x%02x\",\"buttons\": \"%d\",\"ts\": %lu%.*d}]}",
                            snID[0], snID[1], snID[2], snID[3], snID[4], snID[5],  ButtonData, TimeStamp, 3, TimeStampMilliSecondsChange);

  Serial.print("[HTTP] Request: ");
  Serial.println(httpPostRequest);

  httpResponseCode = https.POST(httpPostRequest);
  https.end();

  Serial.print("[HTTP] Response code: ");
  Serial.println(httpResponseCode);
}


//==============================================================================
//
//  bool InsertElemInQueue(char *MacID, char *data, int data_len)
//
//!  This function insert an elem into publish Queue
//
//==============================================================================
bool InsertElemInQueue(char *MacID, char *data, int data_len) {
  QueueElem_t QueueItem;
  memset(&QueueItem, 0x00, sizeof(QueueItem));

  // Serial.printf("[QUEUE] Received Data Length: %d", data_len);
  // Serial.println();

  // Serial.print("[QUEUE] Received Data : ");
  // //for (int i = 0; i < strlen(data); i++)
  // for (int i = 0; i < data_len; i++)
  // {
  //     Serial.printf("%02X ", data[i]);
  // }
  // Serial.println();

  Serial.print("[QUEUE] Putting Element In Queue.......");
  Serial.println();

  memcpy(QueueItem.MacID, MacID, strlen(MacID));
  memcpy(QueueItem.SnID, data, SNIDSTRINGLENGTH);
  memcpy((char*) &QueueItem.ButtonData, (char*) &data[6], BUTTONDATASTRINGLENGTH);
  QueueItem.recvTimeStampSeconds = GetCurrentEpoochTimeStamp();
  QueueItem.recvTimeStampMillisChange = (millis() - localTickAtTimeUpdate) % 1000;

  DataQueue.enqueue(QueueItem);

  Serial.println("[QUEUE] Successful");
  // Serial.print("[QUEUE] MacID: ");
  // Serial.println(QueueItem.MacID);
  // Serial.print("[QUEUE] SNID: ");
  // Serial.println(QueueItem.SnID);
  // Serial.print("[QUEUE] Button Data: ");
  // Serial.println(QueueItem.ButtonData);
  // Serial.println();

  return true;
}

//==============================================================================
//
//  void InitESPNow()
//
//!  This function Init ESP Now with fallback
//
//==============================================================================
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("[ESP NOW] ESPNow Init Success");
  } else {
    Serial.println("[ESP NOW] ESPNow Init Failed");
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
//  void ConfigDeviceAP()
//
//!  This function config Device in AP mode
//
//==============================================================================
void ConfigDeviceAP() {
  String Prefix = "Slave:";
  String Mac = WiFi.macAddress();
  String SSID = Prefix + Mac;
  String Password = "123456789";
  bool result = WiFi.softAP(SSID.c_str(), Password.c_str(), CHANNEL, 0);
  if (!result) {
    Serial.println("[ESP NOW] AP Config failed.");
  } else {
    Serial.println("[ESP NOW] AP Config Success. Broadcasting with AP: " + String(SSID));
  }
}

//==============================================================================
//
//  void OnESPNowDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
//
//!  This function manages callback when data is recv from Master
//
//==============================================================================
void OnESPNowDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  int RecvData[ESP_MESSAGE_SIZE] = {0};
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("[ESP NOW] Last Packet Recv from: ");
  Serial.println(macStr);
  Serial.print("[ESP NOW] Recv Data Length: ");
  Serial.println(data_len);
  Serial.print("[ESP NOW] Last Packet Recv Data: ");
  Serial.printf("%s", data);
  Serial.println("");

  memcpy(RecvData, data, (data_len > ESP_MESSAGE_SIZE)? ESP_MESSAGE_SIZE: data_len);
  // Serial.print("RecvNumPad: ");
  // Serial.println(RecvNumPad);

  InsertElemInQueue(macStr, (char *)&RecvData, ESP_MESSAGE_SIZE);
}

//==============================================================================
//
//  void PrintCurrentTime()
//
//!  This function prints the current Real Time
//
//==============================================================================
void PrintCurrentTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("[TIME] Failed to obtain time");
    return;
  }
  Serial.print("[TIME] ");
  Serial.print(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println();
}

//==============================================================================
//
//  unsigned long GetCurrentEpoochTimeStamp(void);
//
//!  This function returns Current Time in Seconds elapsed since 1970
//
//==============================================================================
unsigned long GetCurrentEpoochTimeStamp(void) {

  unsigned long epochTime;
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("[TIME] Failed to obtain time");
    return(0);
  }
  time(&now);

  // struct tm timeinfo;
  // if(!getLocalTime(&timeinfo)){
  //   Serial.println("[FAIL] Failed to obtain time");
  //   return;
  // }
  // // Formula to calculate the epoch time from the calendar
  // long int epochTime = timeinfo.tm_sec + timeinfo.tm_min*60 + timeinfo.tm_hour*3600 + timeinfo.tm_yday*86400 +(timeinfo.tm_year-70)*31536000 + ((timeinfo.tm_year-69)/4)*86400 -((timeinfo.tm_year-1)/100)*86400 + ((timeinfo.tm_year+299)/400)*86400;

  epochTime = now;

  return epochTime;
}

//==============================================================================
//
//  void CovertUnsignedShortToBinaryMap(uint16_t num, char* OutputBinArray)
//
//!  This function converts the input integer to binary array
//
//==============================================================================

void CovertUnsignedShortToBinaryMap(uint16_t num, char *OutputBinArray) {
  if (num == 0) {
    Serial.println("0");
    memset(OutputBinArray, '0', BYTE_SIZE_SHORT * BIT_SIZE_BYTE);
    return;
  }

  // Stores binary representation of number.
  int binaryNum[BYTE_SIZE_SHORT * BIT_SIZE_BYTE];  // Assuming 16 bit integer.
  int i = 0;

  for (; num > 0;) {
    binaryNum[i] = num % 2;
    OutputBinArray[i] = binaryNum[i];
    i++;
    num /= 2;
  }

  // Printing array in reverse order.
  for (int j = i - 1; j >= 0; j--) {
    Serial.print(binaryNum[j]);
  }
  Serial.println("");
}
