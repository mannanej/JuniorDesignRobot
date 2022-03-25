/*
   Test file to get the camera to work
*/
#include "HUSKYLENS.h"
#include "SoftwareSerial.h"
#include <ESP32Servo.h>

HUSKYLENS huskylens;
Servo speedServo;
Servo directionServo;
int speedServoPin = 33;
int directionServoPin = 32;
int totalSpeed = 50;
//HUSKYLENS green line >> SDA; blue line >> SCL

void setup() {
  Serial.begin(9600);
  Wire.begin();
  while (!huskylens.begin(Wire)) {
    Serial.println(F("Begin failed!"));
    delay(100);
  }
  speedServo.setPeriodHertz(50);
  directionServo.setPeriodHertz(50);
  speedServo.attach(speedServoPin, 1000, 2000);
  directionServo.attach(directionServoPin, 1000, 2000);
  directionServo.write(90);
  calibrateSpeedServo();
}

void loop() {
  if (!huskylens.request()) {
    Serial.println(F("No data, check connection!"));
  } else if (!huskylens.isLearned()) {
    Serial.println(F("Must learn first! Use the LEARN button on camera."));
  } else if (!huskylens.available()) {
    Serial.println(F("Where is the arrow!?"));
  } else {
    //Serial.println(F("###########"));
    while (huskylens.available()) {
      HUSKYLENSResult result = huskylens.read();
      int xValue = result.xTarget;
      //Serial.println(xValue);
      if (xValue < 160) { // Left side of screen, turn left
        int leftAngle = map(xValue, 0, 159, 170, 90);
        directionServo.write(leftAngle);
      } else if (xValue > 160) {  // Right side of screen, turn right
        int rightAngle = map(xValue, 161, 320, 90, 0);
        directionServo.write(rightAngle);
      } else {  // Middle of screen
        directionServo.write(90);
      }
      speedServo.write(totalSpeed);
    }
  }
}

void calibrateSpeedServo() {
  speedServo.write(0);
  delay(5000);
  for (int deg = 0; deg <= 180; deg +=1) {
    speedServo.write(deg);
    delay(10);
  }
  delay(5000);
  return;
}
