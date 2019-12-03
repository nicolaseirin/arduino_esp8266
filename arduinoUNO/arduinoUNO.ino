/* Calibration sketch for HX711 */

#include <HX711.h>  // Library needed to communicate with HX711 https://github.com/bogde/HX711
#include <AccelStepper.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define DOUT_FOOD_CELL   'A1'
#define CLK_FOOD_CELL    'A0'

#define DOUT_WATER_CELL  'A2'
#define CLK_WATER_CELL   'A3'

#define ZERO_WEIGHT 600//300
#define WATER_WEIGHT_LIMIT 20000
#define FOOD_WEIGHT_LIMIT 8000

#define AVERAGE_OF 6


#define ELECTROVALVE 9

// Motor related
#define STEP_PIN 8
#define DIRECTION_PIN 7
#define MOTOR_MAX_SPEED 1000 // in steps/sec
#define MOTOR_ACCELERATION 50
#define MOTOR_THROW_POSITION -280

#define TIME_LIMIT 120000

// NODE MCU Related
#define RX 5
#define TX 6
#define WATER_OUT_TOPIC "waterOutTopic"
#define FOOD_OUT_TOPIC "foodOutTopic"
#define WATER_IN_TOPIC "waterInTopic"
#define FOOD_IN_TOPIC "foodInTopic"
#define CONFIRMATION_WATER_OUT_TOPIC "waterConfirmationOutTopic"
#define CONFIRMATION_FOOD_OUT_TOPIC "foodConfirmationOutTopic"
#define FOOD 'f'
#define WATER 'w'

unsigned long TIMESTAMP_FIRST = 0;
unsigned long TIMESTAMP_LAST = TIME_LIMIT + 1;



AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIRECTION_PIN);
SoftwareSerial nodeMCUSerialCommunicationChannel(RX, TX);
HX711 foodCell;  // Init of library
HX711 waterCell;
const int capacity = JSON_OBJECT_SIZE(3);
StaticJsonDocument<capacity> jsonMessage;

void setup() {
  Serial.begin(9600);
  nodeMCUSerialCommunicationChannel.begin(9600);
  
  initElectroValve();
  initMotor();

  initFoodCell();
  initWaterCell();

  delay(2000);
}

void loop() {
  listenToNodeMCU();
  
  sendData();
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

void sendData() {
  unsigned long ms = millis();
  TIMESTAMP_LAST = ms;

  if (TIMESTAMP_LAST - TIMESTAMP_FIRST > TIME_LIMIT) {
    sendJsonMessageToNodeMCU(WATER_OUT_TOPIC, getWaterLevel());
    sendJsonMessageToNodeMCU(FOOD_OUT_TOPIC, getFoodLevel());
    
    TIMESTAMP_FIRST = ms;
  }
}

long long getWaterLevel() {
  return waterCell.get_value(AVERAGE_OF);
}

long long getFoodLevel() {
  return foodCell.get_value(AVERAGE_OF);
}

void sendJsonMessageToNodeMCU(const char* topic, int amount){
    jsonMessage["topic"] = topic;
    jsonMessage["amount"] = amount;

    while(nodeMCUSerialCommunicationChannel.available() <= 0);

    serializeJson(jsonMessage, nodeMCUSerialCommunicationChannel);
    delay(500);
}

void listenToNodeMCU() {
  DeserializationError error = deserializeJson(jsonMessage, nodeMCUSerialCommunicationChannel);
  if(error) {
    return;
  }

  char whatToDispense = jsonMessage["dispense"];

  if(whatToDispense == WATER) {
    tryToDispenseWater();
  } else if(whatToDispense == FOOD) {
    tryToDispenseFood();
  }
  delay(500);
}

void tryToDispenseFood() {
  long long foodCellWeight = getFoodLevel();
  if (foodCellWeight <  FOOD_WEIGHT_LIMIT) {
    stepper.runToNewPosition(MOTOR_THROW_POSITION);
    delay(1500);
    sendJsonMessageToNodeMCU(CONFIRMATION_FOOD_OUT_TOPIC, foodCellWeight);

     while (true) {
      foodCellWeight = getFoodLevel();
      if (foodCellWeight >= FOOD_WEIGHT_LIMIT) {
         stepper.runToNewPosition(0);
         break;
      }
    }
  } else {
    sendJsonMessageToNodeMCU(CONFIRMATION_FOOD_OUT_TOPIC, -1);
  }
}


void tryToDispenseWater() {
  long long waterCellWeight = getWaterLevel();
  if (waterCellWeight < WATER_WEIGHT_LIMIT) {
    digitalWrite(ELECTROVALVE, HIGH);
    delay(1500);
    sendJsonMessageToNodeMCU(CONFIRMATION_WATER_OUT_TOPIC, waterCellWeight);

    while (true) {
      waterCellWeight = getWaterLevel();
      if (waterCellWeight >= WATER_WEIGHT_LIMIT) {
        digitalWrite(ELECTROVALVE, LOW);
        break;
      }
    }
  } else {
    sendJsonMessageToNodeMCU(CONFIRMATION_WATER_OUT_TOPIC, -1);
  }
}
