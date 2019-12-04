#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

String WATER_OUT_TOPIC = "waterOutTopic";
String FOOD_OUT_TOPIC =  "foodOutTopic";
const char* WATER_IN_TOPIC = "waterInTopic";
const char* FOOD_IN_TOPIC  = "foodInTopic";
String CONFIRMATION_WATER_OUT_TOPIC = "waterConfirmationOutTopic";
String CONFIRMATION_FOOD_OUT_TOPIC = "foodConfirmationOutTopic";

#define FOOD 'f'
#define WATER 'w'

#define RX D6
#define TX D5

#define MQTT_SERVER "192.168.1.7"
#define MQTT_SERVER_PORT 1883
const char* ssid = "neirin-HP-Notebook";
const char* password = "";

PubSubClient client;
SoftwareSerial arduinoSerialCommunicationChannel(RX, TX);
const int capacity = 200;
StaticJsonDocument<capacity> jsonMessage;

void setup() {
  Serial.begin(9600);
  setupWifi();
  client.setServer(MQTT_SERVER, MQTT_SERVER_PORT);
  client.setCallback(callback);
  arduinoSerialCommunicationChannel.begin(9600);
}

void loop() {
  reconnectMQTT();

  listenForArduinoMessages();
}

void setupWifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    
    Serial.println("...");
  };
  Serial.println("Connected to WiFi");
}

void reconnectMQTT() {
  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    reconnect();
  }

  client.loop();
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("trying to connect");
    if (client.connect("esp8266-")) {
      Serial.print("\tMTQQ Connected");

      client.subscribe(WATER_IN_TOPIC);
      client.subscribe(FOOD_IN_TOPIC);
    } else {
      Serial.println("\tFailed.");
      abort();
    }
  }
}


void callback(char* topic, byte * payload, unsigned int length) {
  if (payload[0] == FOOD) {
    tellArduinoToDispense(FOOD);
  } else if (payload[1] == WATER) {
    tellArduinoToDispense(WATER);
  }
}

void listenForArduinoMessages() {

  DeserializationError error = deserializeJson(jsonMessage, arduinoSerialCommunicationChannel);

  if (error) {
    return;
  }

  String topic = jsonMessage["topic"];
  int amount = jsonMessage["amount"];

  client.publish(topic.c_str(), amount + "");
  delay(1000);
}

void tellArduinoToDispense(char what) {
  jsonMessage["dispense"] = what;

  while (arduinoSerialCommunicationChannel.available() <= 0);
  serializeJson(jsonMessage, arduinoSerialCommunicationChannel);
  delay(1000);
}
