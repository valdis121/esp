
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <stDHT.h>
#include <SPI.h>           
#include <MFRC522.h>
#include <MFRC522Extended.h>
constexpr uint8_t RST_PIN = 4; 
constexpr uint8_t SS_PIN = 2;  
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key; 
DHT sens(DHT11);
const int SENSOR = D3;
boolean sign;
boolean danger;
boolean statuslamp;
const int buzzer=D8;
int h;
int t; 
const char* ssid = "fss";
const char* password = "skyrim2002";
#define ORG "pj4rih"
#define DEVICE_TYPE "ESP8266"
#define DEVICE_ID "ESP"
#define TOKEN "aNSfu)nykDrwpjhMNy"
char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;
const char publishTopic[] = "iot-2/evt/status/fmt/json";
const char responseTopic[] = "iotdm-1/response";
const char manageTopic[] = "iotdevice-1/mgmt/manage";
const char updateTopic[] = "iotdm-1/device/update";
const char rebootTopic[] = "iotdm-1/mgmt/initiate/device/reboot";
void callback(char* topic, byte* payload, unsigned int payloadLength);
WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);
int publishInterval = 10000; // 30 seconds
long lastPublishMillis;
byte readCard[4];
byte masterCard[4];
void setup() {
 Serial.begin(115200); Serial.println();
 wifiConnect();
 mqttConnect();
 initManagedDevice();
 pinMode(buzzer, OUTPUT);
 pinMode(16,OUTPUT);
 pinMode(D8,INPUT); 
 statuslamp=false;   
 sign=true;
 danger=false;
 SPI.begin();
  rfid.PCD_Init();
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
 
 
}

void loop() {
check();
if(danger==true){
  analogWrite(buzzer, 100); 
  delay(1000);
  analogWrite(buzzer, 0); 
  delay(1000);
  return;
  }
digitalWrite(16,statuslamp);
if(analogRead(D1)<1023){
Serial.println(analogRead(5));
}

if(analogRead(D1)<1023) {
  if(sign==false){
      statuslamp=!statuslamp; 
      digitalWrite(16,statuslamp);
      delay(50);
  }
 else{
 danger=true;
 publishData();
 }
} 
 h = sens.readHumidity(D3);
 t = sens.readTemperature(D3);
 if (millis() - lastPublishMillis > publishInterval) {
 publishData();
 lastPublishMillis = millis();
 }
 if (!client.loop()) {
 mqttConnect();
 initManagedDevice();
 }
}
bool card(){
  if ( ! rfid.PICC_ReadCardSerial()) {
    return 0;
  }
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = rfid.uid.uidByte[i];
 
  }
   rfid.PICC_HaltA();
  

  delay(2000);

  String content= "";
  byte letter;
  for(byte i = 0; i < rfid.uid.size; i++){
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(rfid.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  if(content.substring(1) != "27 39 9E 35"){
    
    return false;
  }
  else{
    
    return true;  
  }
}
void check(){
   if(!rfid.PICC_IsNewCardPresent()){
    return;  
  }else{
   Serial.print("Checking...");
   if(card()==true){
      
      if(sign==0){
        Serial.println("GOOD BYE");
        statuslamp=false;
       digitalWrite(16,statuslamp);
        sign=true;
      }
      else{
        Serial.println("WELCOME");
        sign=false;
        danger=false;
     }
    }
    else{
      if(sign==true){
        Serial.println("DANGER");
        danger=true;
        publishData();
        return;
      } 
  }
  }
  }
void wifiConnect() {
 Serial.print("Connecting to "); Serial.print(ssid);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
 delay(500);
 Serial.print(".");
 } 
 Serial.print("nWiFi connected, IP address: ");
Serial.println(WiFi.localIP());
}
void mqttConnect() {
 if (!!!client.connected()) {
 Serial.print("Reconnecting MQTT client to "); Serial.println(server);
 while (!!!client.connect(clientId, authMethod, token)) {
 Serial.print(".");
 delay(500);
 }
 Serial.println();
 }
}
void initManagedDevice() {
 if (client.subscribe("iotdm-1/response")) {
 Serial.println("subscribe to responses OK");
 } else {
 Serial.println("subscribe to responses FAILED");
 }
 if (client.subscribe(rebootTopic)) {
 Serial.println("subscribe to reboot OK");
 } else {
 Serial.println("subscribe to reboot FAILED");
 }
 if (client.subscribe("iotdm-1/device/update")) {
 Serial.println("subscribe to update OK");
 } else {
 Serial.println("subscribe to update FAILED");
 }
 StaticJsonBuffer<1000> jsonBuffer;
 JsonObject& root = jsonBuffer.createObject();
 JsonObject& d = root.createNestedObject("d");
 JsonObject& metadata = d.createNestedObject("metadata");
 metadata["publ"] = publishInterval;
 metadata["sign1"] = sign;
 metadata["lamp1"]= statuslamp;
 JsonObject& supports = d.createNestedObject("supports");
 supports["deviceActions"] = true;
 char buff[1000];
 root.printTo(buff, sizeof(buff));
 Serial.println("publishing device metadata:"); Serial.println(buff);
 if (client.publish(manageTopic, buff)) {
 Serial.println("device Publish ok");
 } else {
 Serial.print("device Publish failed:");
 }
}
void publishData() {
  String lamp1;
  String sign1;
  String danger1;
  String payload="";
 int sizelamp=0;
  if(statuslamp==true){
  lamp1="Lamp is on";
  sizelamp=10;
  }
  else{
    sizelamp=11;
     lamp1="Lamp is off";
   }
   int sizesign=0;
  if(sign==true){
  sign1="Signalization is on";
  sizesign=19;
  }
  else{
    sizesign=20;
     sign1="Signalization is off";
   }
    int sizedanger=0;
  if(danger==true){
  danger1="Danger!";
  sizedanger=7;
  }
  else{
    sizedanger=2;
     danger1="OK";
   }
 payload = "{\"d\":{\"t\":";  
 payload += t;
 payload += ",\"danger\":\"";
 for (int i = 0; i < sizedanger; i++)
  payload += String(danger1[i]);
 payload += "\",\"h\":";
  payload += h;
 payload += ",\"sign\":\"";
  for (int i = 0; i < sizesign; i++)
  payload += String(sign1[i]);
 payload += "\",\"lamp\":\"";
 for (int i = 0; i < sizelamp; i++)
  payload += String(lamp1[i]);
 
 payload += "\"}}";
 
 

 Serial.print("Sending payload: "); Serial.println(payload);
 if (client.publish(publishTopic,payload.c_str())) {
 Serial.println("Publish OK"); 
 } else {
 Serial.println("Publish FAILED");
 }
}
void callback(char* topic, byte* payload, unsigned int payloadLength) {
 Serial.print("callback invoked for topic: "); Serial.println(topic);
 if (strcmp (responseTopic, topic) == 0) {
 return; // just print of response for now
 }
 if (strcmp (rebootTopic, topic) == 0) {
 Serial.println("Rebooting...");
 ESP.restart();
 }
 if (strcmp (updateTopic, topic) == 0) {
 handleUpdate(payload);
 }
}
void handleUpdate(byte* payload) {
 StaticJsonBuffer<1000> jsonBuffer;
 JsonObject& root = jsonBuffer.parseObject((char*)payload);
 if (!root.success()) {
 Serial.println("handleUpdate: payload parse FAILED");
 return;
 }
 Serial.println("handleUpdate payload:"); root.prettyPrintTo(Serial);
Serial.println();
 JsonObject& d = root["d"];
 JsonArray& fields = d["fields"];
 for (JsonArray::iterator it = fields.begin(); it != fields.end(); ++it) {
 JsonObject& field = *it;
 const char* fieldName = field["field"];
 if (strcmp (fieldName, "metadata") == 0) {
 JsonObject& fieldValue = field["value"];
 
 if (fieldValue.containsKey("sign1")) {
 sign = fieldValue["sign1"];
 }
 if (fieldValue.containsKey("lamp1")) {
 statuslamp = fieldValue["lamp1"];
 }
 if (fieldValue.containsKey("publ")) {
 publishInterval = fieldValue["publ"];

 Serial.print("publishInterval:"); Serial.println(publishInterval);
 }
 }
 }
} 
