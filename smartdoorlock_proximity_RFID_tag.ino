#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
//#include <AzureIoTHubMQTTClient.h>
#include <PubSubClient.h>
//GMAIL Stuff  SENSITIVE DATA ASK FOR USE

////START NEW
//// example: <myiothub>.azure-devices.net
//const char* iothub_url = "Team12-IoTHub.azure-devices.net";
//
//// this is the id of the device created in Iot Hub
//// example: myCoolDevice
//const char* iothub_deviceid = "SmartLockPi";
//
//// <myiothub>.azure-devices.net/<myCoolDevice>
//const char* iothub_user = "Team12-IoTHub.azure-devices.net/SmartLockPi";
//
//// SAS token should look like "SharedAccessSignature sr=<myiothub>.azure-devices.net%2Fdevices%2F<myCoolDevice>&sig=123&se=456"
//const char* iothub_sas_token = "=+MMtFetGNDIQN1D6QV5oSYSRvZs/wpKsVPViNOTL14k=";
//
//// default topic feed for subscribing is "devices/<myCoolDevice>/messages/devicebound/#""
//const char* iothub_subscribe_endpoint = "";
//
//// default topic feed for publishing is "devices/<myCoolDevice>/messages/events/"
//const char* iothub_publish_endpoint = "devices/SmartLockPi/messages/events";
////END NEW 
#define SS_PIN D4
#define RST_PIN D2
#define sensorPin A0
#define ota_message "test/OTA"

char* ssid = "AlexO_iPhone";
const char* password = "AlexPhone1";
const char* mqtt_server = "m24.cloudmqtt.com";
//const char* topic = "prox";
//const char* user = "ekmzvibr";
//const char* userPassword = "1E5hqZ5FQJsj";
char* topic = "outbound";
const char* user = "lhzsoypk";
const char* userPassword = "-tN8HUBBfZg7";

byte readCard[4];
char* myTags[10] = {};
uint8_t timing=0;
int tagsCount = 0;
String tagID = "";
String status = "Locked";
boolean successRead = false;
boolean correctTag = false;
boolean doorOpened = false;
MFRC522 mfrc522(SS_PIN, RST_PIN);

//WiFiClientSecure espClient;
WiFiClient espClient;
PubSubClient client(espClient);
//AzureIoTHubMQTTClient client(espClient, IOTHUB_HOSTNAME, DEVICE_ID, DEVICE_KEY);

//Sets up the wifi along with reconnect() in order to connect the ESP8266 Module to the MQTT server.
void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  Serial.println("In reconnect...");
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Pound",user,userPassword)) {
      Serial.println("connected");
      //client.subscribe("distanceOutput");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

//Sets up both MQTT server and wifi.  Waits until first card is scanned to set as master card and stores it inside an array.
void setup() {
  Serial.begin(115200);
  setup_wifi();
  //client.setServer(mqtt_server, 12021);
  client.setServer(mqtt_server, 12775);
  client.setCallback(callback);
    pinMode(D0, OUTPUT);
    pinMode(D1, OUTPUT);
  while (!Serial); // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin(); // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  Serial.println(F("Scan card to set as Master Card..."));
  
  // Waits until a master card is scanned
  while (!successRead) {
    
    successRead = getID();
    if ( successRead == true) {

      // The MIFARE PICCs that we use have 4 byte UID
      tagID.toUpperCase();
      myTags[tagsCount] = strdup(tagID.c_str()); // Sets the master tag into position 0 in the array
      tagsCount++;
//      tagID.toUpperCase();

      String rfidmsg = "{\"type\": \"rfid\", \"command\": \"add\", \"payload\": \""+tagID+"\"}";
        if (!client.connected()) {
         reconnect();
       }
      client.loop();
      client.publish("outbound",String(rfidmsg).c_str(),true);
      //client.publish("RFIDAdd","y",true);  //THISONE REMOVE BEFORE FINAL
    }
    yield();
  }
  successRead = false;
  Serial.println(("Master tag Set to "+String(myTags[0])));
}

 //Continously checks to see what the proximity sensor output voltage is and converts it into the distance using the equation on the datasheet.  Concurrently checks to see if RFID cards are being scanned.  If so, the card is compared to the array of stored authenticated cards to see if the lock can be opened.
void loop() {
// Look for new cards
  char msg[10];
  char msgtext[25];
  String themsg;
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
    timing++;

  if (!mfrc522.PICC_IsNewCardPresent()) {
      if (timing%75==0) {
         double outputVoltage = double(analogRead(sensorPin)/(342.0000*1.002924));
         double distance = 4.8966*outputVoltage*1024/5;
         Serial.print(status + "|| Proximity Sensor Output Voltage is  ");
         Serial.println(outputVoltage,6);
         Serial.print("The proximity Distance is ");
         Serial.println(distance,6);
         timing=0;
         sprintf(msgtext, "%e",outputVoltage);
         sprintf(msgtext, "%e",distance);
         //pclient.publish("outputVoltage",String(outputVoltage).c_str(),true);
         //client.publish("distance",String(distance).c_str(),true);
         //client.subscribe("distanceOutput");
         
         client.subscribe("RFIDAdd");
         Serial.println("");
      
      }
    return;
  }
  else
  {
    tagID = "";
    // The MIFARE PICCs that we use have 4 byte UID
    for ( uint8_t i = 0; i < 4; i++) {  //
      readCard[i] = mfrc522.uid.uidByte[i];
      tagID.concat(String(mfrc522.uid.uidByte[i], HEX)); // Adds the 4 bytes in a single String variable
    }
    tagID.toUpperCase();
    client.subscribe("RFIDAdd");
    if (timing%75==0) {
        //client.publish("outbound",String(tagID).c_str(),true);
        
        String rfidmsg = "{\"type\": \"rfid\", \"command\": \"auth\", \"payload\": \""+tagID+"\"}";
        client.publish("outbound",String(rfidmsg).c_str(),true);
        //Serial.println("{“type”:“rfid”,“command”:”auth”,“payload”:”"+tagID+"”}");
      timing=1;
      if (String(myTags[0])==tagID){
        status = "Unlocked";
        Serial.println("Authorized Card: Unlocked");
        //client.publish("cardStatus","Authorized",true);
      }
      else
      {
        status = "Locked";
        Serial.println("Unauthorized Card: Locked");
        //client.publish("cardStatus","Unauthorized",true);
      }
    }
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
}

//Retrives the ID of the RFID card.
uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  tagID = "";
  for ( uint8_t i = 0; i < 4; i++) {  // The MIFARE PICCs that we use have 4 byte UID
    readCard[i] = mfrc522.uid.uidByte[i];
    tagID.concat(String(mfrc522.uid.uidByte[i], HEX)); // Adds the 4 bytes in a single String variable
  }
  tagID.toUpperCase();
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

//Creates the callback function to be used with the subscribe function in client.  Also denotes which LED's should be turned on under what conditions.
void callback(char* topic, byte* payload, unsigned int length) {
  if(String(topic)=="distanceOutput")
  {
    if ((char)payload[0] == 'h') {
      digitalWrite(D0, HIGH);
      digitalWrite(D1, LOW);
    } else if((char)payload[0] == 'l') {
      digitalWrite(D1, HIGH);
      digitalWrite(D0, LOW);
      Serial.println("Something is within Proximity!");
    } else if((char)payload[0] == 'b') {
      digitalWrite(D0, LOW);
      digitalWrite(D1, LOW);
    }
    
  }

  if(String(topic)=="RFIDAdd")
  {
    if ((char)payload[0] == 'y') {
      while (!successRead) {
        successRead = getID();
    if ( successRead == true) {
      tagID.toUpperCase();
      myTags[tagsCount] = strdup(tagID.c_str()); // Sets the master tag into position 0 in the array
      tagsCount++;
//      tagID.toUpperCase();
      String rfidmsg = "{\"type\": \"rfid\", \"command\": \"add\", \"payload\": \""+tagID+"\"}";
        if (!client.connected()) {
         reconnect();
       }
      client.loop();
      client.publish("outbound",String(rfidmsg).c_str(),true);
      //Serial.println(rfidmsg);
    }
      yield();
      }
      successRead = false;
      client.publish("RFIDAdd","n",true);
    }
  }
}
