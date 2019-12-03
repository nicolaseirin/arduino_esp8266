#include <PubSubClient.h>
#include <ESP8266WiFi.h>

String WATER_OUT_TOPIC = "waterOutTopic";
String FOOD_OUT_TOPIC =  "foodOutTopic";

#define WATER_IN_TOPIC "waterInTopic"
#define FOOD_IN_TOPIC "foodInTopic"

String CONFIRMATION_WATER_OUT_TOPIC = "waterConfirmationOutTopic";
String CONFIRMATION_FOOD_OUT_TOPIC = "foodConfirmationOutTopic";

#define MQTT_SERVER "127.0.0.1"
const char* ssid = NULL;
const char* password = NULL;


char* Topic = "inTopic";
PubSubClient client;

char buff[101];
void setup() {
  WiFi.begin(ssid, password);
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte * payload, unsigned int length) {

  String Str = topic;


  if (payload[0] == 'f') {

  }

  if (payload[1] == 'w') {

  }
}

void loop() {
  reconnectMQTT();

  int _size = readFromSerial();
  if (_size != 0) {

    char* message;
    if (compareStrings(CONFIRMATION_WATER_OUT_TOPIC, buff, _size)) {
      
      message = getMessage(buff, CONFIRMATION_WATER_OUT_TOPIC.length(), _size);
      client.publish(CONFIRMATION_WATER_OUT_TOPIC.toCharArray(), message);

    } else if (compareStrings(CONFIRMATION_FOOD_OUT_TOPIC, buff, _size)) {
      
      
      message = getMessage(buff, CONFIRMATION_FOOD_OUT_TOPIC.length(), _size);
      client.publish(CONFIRMATION_FOOD_OUT_TOPIC.toCharArray(), message);
      
    } else if (compareStrings(WATER_OUT_TOPIC, buff, _size)) {
      
      message = getMessage(buff, WATER_OUT_TOPIC.length(), _size);
      client.publish(WATER_OUT_TOPIC.toCharArray(), message);
      
    } else if (compareStrings(FOOD_OUT_TOPIC, buff, _size)) {
      
      message = getMessage(buff, FOOD_OUT_TOPIC.length(), _size);
      client.publish(FOOD_OUT_TOPIC.toCharArray(), message);
    }
  }
}

void reconnectMQTT() {

  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    reconnect();
  }

  client.loop();
  delay(20);
}

void reconnect() {
  while (!client.connected()) {

    if (client.connect("esp8266-")) {

      Serial.print("\tMTQQ Connected");

      client.subscribe(WATER_IN_TOPIC);
      client.subscribe(FOOD_IN_TOPIC);
    }
    else
    {
      Serial.println("\tFailed.");
      abort();
    }
  }
}

int readFromSerial() {
  if (Serial.available() <= 0) {
    return 0;
  }

  memset(buff, 0, 101);
  return Serial.readBytesUntil('\n', buff, 100);
}

bool compareStrings(String toCompare, char* in, int size) {
  int i = 0;
  int j = 0;
  while (i < toCompare.length() && j < size) {
    if (toCompare[i++] != in[j++]) {
      return false;
    }
  }
  return true;
}

char* getMessage(char* buff, int from, int to) {
  String toReturn = "";
  while (from < to) {
    toReturn = toReturn + buff[from++];
  }   return toReturn.toCharArray();
}
