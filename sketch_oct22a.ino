/* Calibration sketch for HX711 */

#include <HX711.h>  // Library needed to communicate with HX711 https://github.com/bogde/HX711
#include <AccelStepper.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

//MQTT CONNECTION
#define MQTT_SERVER "192.168.1.117"
const char* ssid = "YOURWIFI";
const char* password = "PASSWORD";

char* Topic = "inTopic";
WiFiClient wifiClient;

#define DOUT_FOOD_CELL   A1
#define CLK_FOOD_CELL    A0

#define DOUT_WATER_CELL  A2
#define CLK_WATER_CELL   A3

#define ZERO_WEIGHT 600//300
#define WATER_WEIGHT_LIMIT 20000
#define FOOD_WEIGHT_LIMIT 8000

#define AVERAGE_OF 6

#define ELECTROVALVE 9

#define STEP_PIN 8
#define DIRECTION_PIN 7

#define MOTOR_MAX_SPEED 1000 // in steps/sec
#define MOTOR_ACCELERATION 50

#define MOTOR_THROW_POSITION -280

#define TIME_LIMIT 120000
unsigned long TIMESTAMP_FIRST = 0;
unsigned long TIMESTAMP_LAST = TIME_LIMIT + 1;

#define WATER_OUT_TOPIC "waterOutTopic"
#define FOOD_OUT_TOPIC "foodOutTopic"

#define WATER_IN_TOPIC "waterInTopic"
#define FOOD_IN_TOPIC "foodInTopic"

#define CONFIRMATION_WATER_OUT_TOPIC "waterConfirmationOutTopic"
#define CONFIRMATION_FOOD_OUT_TOPIC "foodConfirmationOutTopic"

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIRECTION_PIN);

HX711 foodCell;  // Init of library
HX711 waterCell;

void setup() {

  Serial.begin(9600);

  initElectroValve();
  initMotor();

  initFoodCell();
  initWaterCell();

  WiFi.begin(ssid, password);

  delay(2000);
}

void loop() {

  reconnectMQTT();

  Serial.print("Valor de lectura food cell:  ");
  Serial.println(foodCell.get_value(AVERAGE_OF));
  delay(10);

  Serial.print("Valor de lectura water cell:  ");
  Serial.println(waterCell.get_value(AVERAGE_OF));
  delay(10);

  handleWater();
  delay(10);

  handleFood();
  delay(10);

  sendData();
}

void reconnectMQTT() {

  if (!client.connected() && WiFi.status() == 3) {
    reconnect();
  }

  client.loop();

  delay(20);
}

void reconnect() {

  if (WiFi.status() != WL_CONNECTED) {

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }

  if (WiFi.status() == WL_CONNECTED) {

    while (!client.connected()) {

      String clientName;
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);

      if (client.connect((char*) clientName.c_str())) {

        Serial.print("\tMTQQ Connected");

        client.subscribe(WATER_IN_TOPIC);
        client.subscribe(FOOD_IN_TOPIC);

        client.setCallback(callback);
      }
      else
      {

        Serial.println("\tFailed.");
        abort();
      }
    }
  }
}

void initElectroValve() {
  pinMode(ELECTROVALVE, OUTPUT);
  digitalWrite(ELECTROVALVE, LOW);
}

void initMotor() {
  stepper.setMaxSpeed(MOTOR_MAX_SPEED );
  stepper.setAcceleration(MOTOR_ACCELERATION);
}

void initFoodCell() {
  foodCell.begin(DOUT_FOOD_CELL, CLK_FOOD_CELL);
  foodCell.set_scale(); //La escala por defecto es 1
  foodCell.tare(25);  //El peso actual es considerado Tara.
}

void initWaterCell() {
  waterCell.begin(DOUT_WATER_CELL, CLK_WATER_CELL);
  waterCell.set_scale(); //La escala por defecto es 1
  waterCell.tare(25);  //El peso actual es considerado Tara.
}

void handleWater() {
  long long waterCellWeight = waterCell.get_value(AVERAGE_OF);
  if (waterCellWeight <= ZERO_WEIGHT) {

    digitalWrite(ELECTROVALVE, HIGH);
    Serial.print("Encender bomba de agua.");
  }

  if (waterCellWeight > WATER_WEIGHT_LIMIT) {

    digitalWrite(ELECTROVALVE, LOW);
    Serial.print("Apagar bomba de agua.");
  }
}

void handleFood() {
  long long foodCellWeight = foodCell.get_value(AVERAGE_OF);
  if (foodCell.get_value(AVERAGE_OF) <= ZERO_WEIGHT) {
    stepper.runToNewPosition(MOTOR_THROW_POSITION);
  }

  if (foodCell.get_value(AVERAGE_OF) > FOOD_WEIGHT_LIMIT) {
    stepper.runToNewPosition(0);
  }
}

void callback(char* topic, byte * payload, unsigned int length) {

  String Str = topic;

  if (payload[0] == 'F') {
    tryToDispenseFood();
  }

  if (payload[1] == 'W') {
    tryToDispenseWater();
  }
}

void tryToDispenseFood() {

  while (true) {

    long long foodCellWeight = foodCell.get_value(AVERAGE_OF);
    if (foodCell.get_value(AVERAGE_OF) <  FOOD_WEIGHT_LIMIT) {

      client.publish(CONFIRMATION_FOOD_OUT_TOPIC, "Tirando Comida...");

      stepper.runToNewPosition(MOTOR_THROW_POSITION);
    }
    else
    {
      client.publish(CONFIRMATION_FOOD_OUT_TOPIC, "No es posible dispensar comida, el contenedor se encuentra al limite de su capacidad.");
      break;
    }

    if (foodCell.get_value(AVERAGE_OF) > FOOD_WEIGHT_LIMIT) {
      stepper.runToNewPosition(0);

      client.publish(CONFIRMATION_FOOD_OUT_TOPIC, "Tirando Comida...");
      break;
    }
  }
}

void tryToDispenseWater() {

  while (true) {

    long long waterCellWeight = waterCell.get_value(AVERAGE_OF);

    if (waterCellWeight <= WATER_WEIGHT_LIMIT) {

      digitalWrite(ELECTROVALVE, HIGH);

      client.publish(CONFIRMATION_WATER_OUT_TOPIC, "Encender bomba de agua.");

      Serial.print("Encender bomba de agua.");
    }
    else
    {
      client.publish(CONFIRMATION_WATER_OUT_TOPIC, "Contenedor de agua al limite de su capacidad, no es posible dispensar agua.");
      break;
    }

    if (waterCellWeight > WATER_WEIGHT_LIMIT) {

      digitalWrite(ELECTROVALVE, LOW);

      client.publish(CONFIRMATION_WATER_OUT_TOPIC, "Apagar bomba de agua.");

      Serial.print("Apagar bomba de agua.");
      break;
    }
  }
}

void sendData() {

  unsigned long ms = millis();
  TIMESTAMP_LAST = ms;

  if (TIMESTAMP_LAST - TIMESTAMP_FIRST > TIME_LIMIT) {

    client.publish(WATER_OUT_TOPIC, getWaterLevel() + "");

    client.publish(FOOD_OUT_TOPIC, getFoodLevel() + "");

    TIMESTAMP_FIRST = ms;
  }
}

long long getWaterLevel() {
  return waterCell.get_value(AVERAGE_OF);
}

long long getFoodLevel() {
  return foodCell.get_value(AVERAGE_OF);
}
