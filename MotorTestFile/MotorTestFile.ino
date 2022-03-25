/*
 * Test file to get the two servo motors to work
 */

#include <ESP32Servo.h>
Servo speedServo;
Servo directionServo;
int speedServoPin = 33;
int directionServoPin = 32;

void setup()                                  // this states the name of the function
{
  Serial.begin(9600);
  speedServo.setPeriodHertz(50);
  directionServo.setPeriodHertz(50);
}

void loop()                                   // this tells the function inside to loop
{
  speedServo.attach(speedServoPin, 1000, 2000);
  directionServo.attach(directionServoPin, 1000, 2000);

  // Test the direction servo
  for (int deg = 90; deg <= 170; deg += 1) {
    directionServo.write(deg);
    delay(10);
  }
  delay(1000);
  for (int deg = 170; deg >= 90; deg -= 1) {
    directionServo.write(deg);
    delay(10);
  }
  for (int deg = 90; deg >= 10; deg -= 1) {
    directionServo.write(deg);
    delay(10);
  }
  delay(1000);
  for (int deg = 10; deg <= 90; deg += 1) {
    directionServo.write(deg);
    delay(10);
  }

  // Testing the speed servo
  speedServo.write(0);
  delay(5000);
  for (int deg = 0; deg <= 180; deg +=1) {
    speedServo.write(deg);
    delay(10);
  }
  delay(5000);
}
