#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

String WATER_OUT_TOPIC = "waterOutTopic";
String FOOD_OUT_TOPIC =  "foodOutTopic";
#define WATER_IN_TOPIC "waterInTopic"
#define FOOD_IN_TOPIC "foodInTopic"
String CONFIRMATION_WATER_OUT_TOPIC = "waterConfirmationOutTopic";
String CONFIRMATION_FOOD_OUT_TOPIC = "foodConfirmationOutTopic";
#define FOOD 'f'
#define WATER 'w'

#define RX D6
#define TX D5

#define MQTT_SERVER "127.0.0.1"
#define MQTT_SERVER_PORT 1883
const char* ssid = NULL;
const char* password = NULL;

PubSubClient client;
SoftwareSerial arduinoSerialCommunicationChannel(RX, TX);
const int capacity = JSON_OBJECT_SIZE(3);
StaticJsonDocument<capacity> jsonMessage;

void setup() {
  WiFi.begin(ssid, password);
  client.setServer(MQTT_SERVER, MQTT_SERVER_PORT);
  client.setCallback(callback);
  arduinoSerialCommunicationChannel.begin(9600);
}

void loop() {
  reconnectMQTT();

  listenForArduinoMessages();
}

void reconnectMQTT() {
  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    reconnect();
  }

  client.loop();
}

void reconnect() {
  while (!client.connected()) {
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

  if(error) {
    return;
  }

  const char* topic = jsonMessage["topic"];
  const char* amount = jsonMessage["amount"];

  client.publish(topic, amount);
  delay(500);
}

void tellArduinoToDispense(char what) {
  jsonMessage["dispense"] = what;

  while(arduinoSerialCommunicationChannel.available() <= 0);
  serializeJson(jsonMessage, arduinoSerialCommunicationChannel);
  delay(500);
}
