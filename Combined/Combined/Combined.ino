#include <HUSKYLENS.h>
#include <SoftwareSerial.h>
#include <ESP32Servo.h>
#include "TOOBluetooth.h"
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <StaticThreadController.h>
#include <Thread.h>
#include <ThreadController.h>
HUSKYLENS huskylens;
Servo speedServo;
Servo directionServo;
int speedServoPin = 33;
int directionServoPin = 32;
int totalSpeed = 50;
int servoPosition = 90;
float SERVO_POSITION_STEP = 10.0;
int POISONING_NUMBER = 1;
int averageArray[6] = {160, 160, 160, 160, 160, 160};
int arrayCount = 0;
int xValue = 0;
int arrayLen=6;
int bottomX = 0;
int bottomY = 0;
int yOrigin;
Thread currentUpdateThread = Thread();
Thread camThread = Thread();

void runIAPoll();
void calibrateSpeedServo();
bool StartIAThread(int interval) {
  //Serial.println("Starting watchdog...");


  currentUpdateThread.enabled = true;
  currentUpdateThread.setInterval(interval);
  //  watchdogThread.ThreadName = "Bluetooth Watchdog";
  currentUpdateThread.onRun(runIAPoll);
  currentUpdateThread.run();
  return true;
}

bool StartCamThread(int interval) {
  //Serial.println("Starting watchdog...");


  camThread.enabled = true;
  camThread.setInterval(interval);

  camThread.onRun(runCamera);
  camThread.run();
  return true;
}

Adafruit_INA219 ina219;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  speedServo.setPeriodHertz(50);
  directionServo.setPeriodHertz(50);
  speedServo.attach(speedServoPin, 1000, 2000);
  directionServo.attach(directionServoPin, 1000, 2000);
  directionServo.write(90);
  Serial.println("Calibrating servo...");
  calibrateSpeedServo();
  BTStart();
  BTStartWatchdog(5);
  Serial.println("Started!");
  delay(1000); // Wait for cooldown
  // Try to init INA219
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
  }
  Wire.begin();
  while (!huskylens.begin(Wire)) {
    Serial.println(F("Begin failed!"));
    delay(100);
  }
  StartIAThread(500);
  StartCamThread(1);
}
void runCamera() {
  if (huskylens.available()) {
    HUSKYLENSResult result = huskylens.read();

    xValue = result.xTarget;
    yOrigin = result.yOrigin;
  } else {
    BTPrintln("Poisoning...");
    xValue = 160;
  }

}
float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float loadvoltage = 0;
float power_mW = 0;
String stringFix = "";
float average = 0;

void runIAPoll() {
  // BTPrintln("POLL");
  stringFix = "";
  stringFix.concat(shuntvoltage);
  stringFix.concat(busvoltage);
  stringFix.concat(current_mA);
  stringFix.concat(power_mW);
  stringFix.concat(loadvoltage);
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);

  BTPrintln(stringFix);
}

void loop() {

  //Serial.println("Sent packet!");
  watchdogTick();
  if (isLocked()) {
    while (1) {
      speedServo.write(0);
      speedServo.write(0);
      flashWD();
      delay(2000);
      Serial.println("WARN: FAILURE LOCKOUT (BT DISCONNECT)");

    }
  }
  if (currentUpdateThread.shouldRun()) {
    currentUpdateThread.run();
  }

  bottomY = yOrigin;
  boolean failsafe = false;
  if (!huskylens.request()) {
    Serial.println(F("No data, check connection!"));
  } else if (!huskylens.isLearned()) {
    Serial.println(F("Must learn first! Use the LEARN button on camera."));
  } else if (!huskylens.available()) {
    BTPrintln("Poisoning!");
    //failsafe = true;
    for(int i = 0; i < POISONING_NUMBER; i++) {
      averageArray[arrayCount] = 160;
      arrayCount++;
      if (arrayCount == arrayLen-1) {
        arrayCount = 0;
      }  
    }
  } else {
    //delay(5);
    if (camThread.shouldRun()) {
      camThread.run();

    }

    //Serial.println(xValue);
    averageArray[arrayCount] = xValue;
    long sum = 0;
    for (int i = 0; i <= arrayLen-1; i++) {
      sum += averageArray[i];
    }
    average = sum / arrayLen;
    arrayCount++;
  }
  BTPrintln(average);
  if (average < 160) { // Left side of screen, turn left
    int leftAngle = map(average, 16, 159, 140, 90);
    directionServo.write(leftAngle);
  } else if (average > 160) {  // Right side of screen, turn right
    int rightAngle = map(average, 161, 240, 90, 40);
    directionServo.write(rightAngle);
  } else {  // Middle of screen
    directionServo.write(90);
  }
  if (arrayCount == arrayLen-1) {
    arrayCount = 0;
  }
  speedServo.write(totalSpeed);
}

/*
  //Serial.println(F("###########"));
  // "ignorance" constant, essentially how much change there needs to be for it to turn. tweak as needed
  int CHANGE_THRESHOLD = 3;
  while (huskylens.available()) {
  HUSKYLENSResult result = huskylens.read();
  int xStart = result.xOrigin;
  int xEnd = result.xTarget;
  int xValue = (xStart + xEnd) / 2;
  // BTPrintln(xValue);
  int angleTarget = 90;
  //Serial.println(xValue);
  if (xValue < 160) { // Left side of screen, turn left
    angleTarget = map(xValue, 16, 159, 180, 90);
  } else if (xValue > 160) {  // Right side of screen, turn right
    angleTarget = map(xValue, 161, 280, 90, 0);
  }
  if ((angleTarget - servoPosition) > CHANGE_THRESHOLD) {
    servoPosition += SERVO_POSITION_STEP;
  } else if ((servoPosition - angleTarget) > CHANGE_THRESHOLD) {
    servoPosition -= SERVO_POSITION_STEP;
  }
  directionServo.write(servoPosition);
  speedServo.write(totalSpeed);
  }
  }
*/


void calibrateSpeedServo() {
  speedServo.write(0);
  delay(5000);
  for (int deg = 0; deg <= 180; deg += 20) {
    speedServo.write(deg);
    delay(10);
  }
  delay(2000);
  speedServo.write(0);
  return;
}
