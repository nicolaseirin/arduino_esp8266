/* Calibration sketch for HX711 */

#include <HX711.h>  // Library needed to communicate with HX711 https://github.com/bogde/HX711
#include <AccelStepper.h>

#define DOUT_FOOD_CELL   'A1'
#define CLK_FOOD_CELL    'A0'

#define DOUT_WATER_CELL  'A2'
#define CLK_WATER_CELL   'A3'

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



AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIRECTION_PIN);

HX711 foodCell;  // Init of library
HX711 waterCell;

void setup() {

  Serial.begin(9600);

  initElectroValve();
  initMotor();

  initFoodCell();
  initWaterCell();

  delay(2000);
}

void loop() {

  Serial.print("Valor de lectura food cell:  ");
  Serial.println(foodCell.get_value(AVERAGE_OF));
  delay(10);

  Serial.print("Valor de lectura water cell:  ");
  Serial.println(waterCell.get_value(AVERAGE_OF));
  delay(10);

  //handleWater();
  delay(10);

  //handleFood();
  delay(10);

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


/*
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
*/

void callback(char* topic, byte * payload, unsigned int length) {

  String Str = topic;


  if (payload[0] == 'f') {
    tryToDispenseFood();
  }

  if (payload[1] == 'w') {
    tryToDispenseWater();
  }
}

void tryToDispenseFood() {

  long long foodCellWeight = getFoodLevel();
  if (foodCellWeight <  FOOD_WEIGHT_LIMIT) {
    
    stepper.runToNewPosition(MOTOR_THROW_POSITION);
    delay(15000);
    client.publish(CONFIRMATION_FOOD_OUT_TOPIC, foodCellWeight + "");

     while (true) {
      foodCellWeight = getFoodLevel();
      if (foodCellWeight >= FOOD_WEIGHT_LIMIT) {
         stepper.runToNewPosition(0);
        break;
      }
    }
  }
  else
  {
    client.publish(CONFIRMATION_FOOD_OUT_TOPIC, "-1");
  }
}


void tryToDispenseWater() {

  long long waterCellWeight = getWaterLevel();
  if (waterCellWeight < WATER_WEIGHT_LIMIT) {

    digitalWrite(ELECTROVALVE, HIGH);
    delay(15000);
    client.publish(CONFIRMATION_WATER_OUT_TOPIC, waterCellWeight + "");

    while (true) {
      waterCellWeight = getWaterLevel();
      if (waterCellWeight >= WATER_WEIGHT_LIMIT) {
        digitalWrite(ELECTROVALVE, LOW);
        break;
      }
    }
  }
  else
  {
    client.publish(CONFIRMATION_WATER_OUT_TOPIC, "-1");
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
