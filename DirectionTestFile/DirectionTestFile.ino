/*
   Test file to get the Direction Servo to work
   Documentation claims angle is from 10 to 170 with 90 at center. True?
   ANSWER: FALSE. The servo will move all the way from 0 to 180. Center is indeed 90 though.
   Documentation claims camera screen goes from 0 to 320. True?
   ANSWER: FALSE. The camera screen only seems to register from 16 to 280.
*/
#include "HUSKYLENS.h"
#include "SoftwareSerial.h"
#include <ESP32Servo.h>

HUSKYLENS huskylens;
Servo directionServo;
int directionServoPin = 32;
//HUSKYLENS green line >> SDA; blue line >> SCL

void setup() {
  Serial.begin(9600);
  Wire.begin();
  while (!huskylens.begin(Wire)) {
    Serial.println(F("Begin failed!"));
    delay(100);
  }
  directionServo.setPeriodHertz(50);
  directionServo.attach(directionServoPin, 1000, 2000);
  directionServo.write(90);
  testTurn();
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
      Serial.println(xValue);
      if (xValue < 160) { // Left side of screen, turn left
        int leftAngle = map(xValue, 0, 159, 170, 90);
        directionServo.write(leftAngle);
      } else if (xValue > 160) {  // Right side of screen, turn right
        int rightAngle = map(xValue, 161, 320, 90, 0);
        directionServo.write(rightAngle);
      } else {  // Middle of screen
        directionServo.write(90);
      }
    }
  }
}

void testTurn() {
  Serial.println("Time to check our angles!");
  delay(5000);
  Serial.println("First, 90 to 180");
  for (int i = 90; i <= 180; i = i + 10) {
    Serial.println(i);
    directionServo.write(i);
    delay(2000);
  }
  delay(5000);
  Serial.println("Now, 90 to 0");
  for (int i = 90; i >= 0; i = i - 10) {
    Serial.println(i);
    directionServo.write(i);
    delay(2000);
  }
}
