#include <LinkedList.h>

/*
    This is up to date as of 1/19/2022
    TEAM TOO
    Last Modified by: Zackery Painter
    THIS PROGRAM WORKS!!!

*/
#include <HUSKYLENS.h>
#include <SoftwareSerial.h>
#include <ESP32Servo.h>
#include "TOOBluetooth.h"
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <StaticThreadController.h>
#include <Thread.h>
#include <ThreadController.h>
#include "soc/rtc_wdt.h"

/*
   Start defining first round of variables



*/
HUSKYLENS huskylens; //Create Camera
Servo speedServo; //Speed Servo
Servo directionServo; //Direction Servo
//Define parameters for the servos

int speedServoPin = 33;
int directionServoPin = 32;
int totalSpeed = 50;
int servoPosition = 90;
float SERVO_POSITION_STEP = 10.0;
int POISONING_NUMBER = 1;
int averageArray[6] = {160, 160, 160, 160, 160, 160};

//Define Bluetooth Queueing information

//String sendQueue[70] = {""};
//int sendIndex = 0;
//int queueLen = 70;
String BTName = "MANNANEJ-STG4";

// Define Averager parameters
int arrayCount = 0;
int xValue = 0;
int arrayLen = 6;
int bottomX = 0;
int bottomY = 0;
int yOrigin;
float average = 0;
bool waitToSend = false;
//Start Button
int dumpButton = 35;

//Define DEBUG enable parameters
int waitTime = 1000; //DEBUG Delay time in MS
int endTime = 120000; // Final lockout time in milliseconds
int startMillis = 0;
int currently_sending = 0;
bool DEBUG;
void runCamera();
//Create Tasks and lockout information
TaskHandle_t readBltTsk;
TaskHandle_t WriteBltTsk;
Thread currentUpdateThread = Thread();
Thread camThread = Thread();
void runIAPoll(); //IA219 thread template
void calibrateSpeedServo(); //Servo function template
bool startLockout = true; //Set to false to disable starting in lockout mode
void writeBT(void * parameters);
bool DEAD = false;
LinkedList<String> sendQueue= LinkedList<String>();
//Define Current chip setup
Adafruit_INA219 ina219;


/*
   Start the IA219 polling thread
   Takes an interval as an integer
   Returns true if it runs properly
*/
bool StartIAThread(int interval) {
  //Serial.println("Starting watchdog...");


  currentUpdateThread.enabled = true;
  currentUpdateThread.setInterval(interval);
  //  watchdogThread.ThreadName = "Bluetooth Watchdog";
  currentUpdateThread.onRun(runIAPoll);
  currentUpdateThread.run();
  return true;
}
/*
   Starts the Camera polling thread
   Takes an interval
   Returns true if it runs properly

*/
bool StartCamThread(int interval) {
  //Serial.println("Starting watchdog...");


  camThread.enabled = true;
  camThread.setInterval(interval);

  camThread.onRun(runCamera);
  camThread.run();
  return true;
}

/*
   Start setting up the device


*/
void setup() {

  // put your setup code here, to run once:
  Serial.begin(115200); //Setup serial
  // Define Servos and speed motor
  speedServo.setPeriodHertz(50);
  directionServo.setPeriodHertz(50);
  speedServo.attach(speedServoPin, 1000, 2000);
  directionServo.attach(directionServoPin, 1000, 2000);
  directionServo.write(90);
  //Serial.println("Calibrating servo...");
  //Now connect to Bluetooth
  BTStart(BTName);
  BTStartWatchdog(5);

  Serial.println("Started!");

  // Try to init INA219
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
  }
  // Start running the Camera
  Wire.begin();
  while (!huskylens.begin(Wire)) {
    Serial.println(F("Begin failed!"));
    delay(100);
  }
  // Start all threads
  StartIAThread(500);
  StartCamThread(1);
  // Define the start / dump button
  pinMode(dumpButton, INPUT);

  xTaskCreatePinnedToCore(
    writeBT,   /* Task function. */
    "WriteBTCore",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    5,           /* priority of the task */
    &WriteBltTsk,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  vTaskSuspend(WriteBltTsk); //Suspend the task to prevent watchdog errors
  //Create the BT input code on core 0

}
/*
   Call this function to add to the Bluetooth Send Queue


*/
void addtoSendQueue(String toAdd, String sendType) {

  if (toAdd.equals(NULL) || toAdd.equals("")) {
    return;
  }
  if (sendType.equals("Info")) {
    toAdd = "?:" + toAdd;

  } else if (sendType.equals("Data")) {
    toAdd = "#:" + toAdd;

  } else if (sendType.equals("Command")) {
    toAdd = "!:" + toAdd;
  }
  toAdd = toAdd + ":" ;
  toAdd+= String(millis() - startMillis)+":"+String(millis());
  sendQueue.add(0,toAdd+":*");
  }

/*
   This function handles writing to Bluetooth

*/
void writeBT(void * parameter) {

  for (;;) {
    rtc_wdt_feed();
    vTaskDelay(pdMS_TO_TICKS(100));

    char datIn = readData();
    if (datIn == 'D') {

      DEBUG = true;

      while (!BTisReady()) {
        rtc_wdt_feed();
        vTaskDelay(pdMS_TO_TICKS(100));
      }
      BTPrintln("?:=========BEGIN DUMP=========*");
      BTPrint("?:Current Runtime: *");
      BTPrintln("?:" + String(millis()) + "*");
      BTPrint("?:Started Cycle at: *");
      BTPrintln("?:" + String(startMillis) + "*");
      BTPrint("?:Current Speed:*");
      BTPrintln("?:" + String(totalSpeed) + "*");
      BTPrint("?:Current Averaged Angle: *");
      BTPrintln("?:" + String(average) + "*");
      BTPrint("?:Current CORE:*");
      BTPrintln("?:" + String(xPortGetCoreID()) + "*");
      BTPrintln("?:=========END DUMP=========*");
      BTPrintln("?:=========BEGIN LIVE DUMP=========*");
      waitToSend = true;

    } else if (datIn == 'K') {
      DEAD = true;
      DEBUG = true;
      speedServo.write(0);
      totalSpeed = 0;
      //delay(1000);
      while (!BTisReady()) {
        rtc_wdt_feed();
        vTaskDelay(pdMS_TO_TICKS(100));
      }
      BTPrintln("?:=========BEGIN DUMP=========*");
      BTPrint("Current Runtime: *");
      BTPrintln("?:" + String(millis()) + "*");
      BTPrint("?:Started Cycle at: *");
      BTPrintln("?:" + String(startMillis) + "*");
      BTPrint("?:Current Speed:*");
      BTPrintln("?:" + String(totalSpeed) + "*");
      BTPrint("?:Current Averaged Angle: *");
      BTPrintln("?:" + String(average) + "*");
      BTPrint("?:Current CORE:*");
      BTPrintln("?:" + String(xPortGetCoreID()) + "*");
      BTPrintln("?:=========END DUMP=========*");
      BTPrintln("?:NO LIVE DEBUG: ROBOT IS DEAD*");
      waitToSend = true;

    } else if (datIn == 'R') {
      waitToSend = true;

      BTPrintln("?=========END DEBUG=========*");
      DEBUG = false;
      totalSpeed = 50;
      waitToSend = true;

    }
    else {
      Serial.print("Received invalid command Sending Data: ");
      Serial.println(datIn);

      if (sendQueue.size()>0) {
        String sending=sendQueue.pop();
        // for (int i = 0; i <= sendIndex; i++) {


        //        while (!BTisReady()) {
        //
        //          Serial.println("Waiting for BT");
        //          rtc_wdt_feed();
        //          vTaskDelay(pdMS_TO_TICKS(100));
        //        }
        if (sending.equals(NULL) ||sending.equals("")) {
          Serial.println("Empty queue object, throwing out...");

        } else {
          delay(500);

          Serial.print("Sending: ");
          
          Serial.println(sending);
          if (!BTForceSend(sending)) {
            Serial.println("Failed to send!");
          } else {

          }
          //Serial.println("Giving control to Read");

        }
        // yield();
        //}
        //sendIndex = 0;
      } else {
        //currently_sending = 0;
        Serial.println("Nothing to send...");
      }
    }

  }
}
/*
   This function is the task run on core 0
   It reads from Bluetooth and handles sending

*/
/*
   After the startup button is pressed, call this to start the robot.


*/
void finishStartup() {
  // BTPrintln(startMillis);
  calibrateSpeedServo();
  // Wait for S to show up on the robot from Bluetooth
  while (readData() != 'S') {
    startLockout = false;
    // addtoSendQueue("Waiting For Start");
    Serial.println("Waiting for Start");
  }
  //addtoSendQueue("Started!");
  delay(100);
  Serial.println("Start!");
  startMillis = millis();
  vTaskResume(WriteBltTsk);


}
/*
   Run the Camera thread if we are ready


*/
void runCamera() {
  if (huskylens.available()) {
    HUSKYLENSResult result = huskylens.read();

    xValue = result.xTarget;
    yOrigin = result.yOrigin;
  } else {
    //  BTPrintln("Poisoning...");
    xValue = 160;
  }

}
float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float loadvoltage = 0;
float power_mW = 0;
String stringFix = "";
/*
   Runn the camera polling if we are ready


*/
void runIAPoll() {
  Serial.println("Current Poll");
  // BTPrintln("POLL");
  stringFix = "";
  stringFix.concat(shuntvoltage);
  stringFix +=  ":";
  stringFix.concat(busvoltage);
  stringFix +=  ":";

  stringFix.concat(current_mA);
  stringFix +=  ":";

  stringFix.concat(power_mW);
  stringFix +=  ":";

  stringFix.concat(loadvoltage);
  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  //while(!BTisReady);
  addtoSendQueue(stringFix, "Data");
  Serial.print("Current: ");
  Serial.println(current_mA);
}
/*
   Check if BT locked us out and stay in that state.

*/
void checkLockout() {
  watchdogTick();
  if (isLocked() || DEAD) {
    while (1) {
      speedServo.write(0);
      speedServo.write(0);
      flashWD();
      delay(2000);
      Serial.println("WARN: FAILURE LOCKOUT (BT DISCONNECT)");

    }
  }
}
/*
   Main loop

*/
void loop() {


  /*
     Stay in the locked-out state if we haven't pressed the start button yet

  */
  while (startLockout) {
    checkLockout();
    if (digitalRead(dumpButton) == LOW) {
      startLockout = false;
      finishStartup();
    }
  }
  //Check for lockout conditions
  checkLockout();
  //Check if our time is up.
  if ((millis() - startMillis) >= endTime) {
    for (int i = totalSpeed; i >= 0; i--) {
      speedServo.write(i);
    }
    speedServo.write(0);
    while (1) {
      speedServo.write(0);
    }
  }
  //DEBUG code
  if (DEBUG) {
    Serial.println(">>SENDING STATE!");
    addtoSendQueue("MAIN LOOP", "Info");
    delay(waitTime);
  }

  // Run threads if we need to
  if (currentUpdateThread.shouldRun()) {
    currentUpdateThread.run();
    if (DEBUG) {
      Serial.println(">>SENDING STATE!");
      addtoSendQueue("RUNNING CURRENT UPDATE THREAD", "Info");
      delay(waitTime);

    }
  }
  /*
     Below this line is Line following code


  */
  bottomY = yOrigin;
  boolean failsafe = false;
  if (!huskylens.request()) {
    Serial.println(F("No data, check connection!"));
  } else if (!huskylens.isLearned()) {
    Serial.println(F("Must learn first! Use the LEARN button on camera."));
  } else if (!huskylens.available()) {
    //  BTPrintln("Poisoning!");
    if (DEBUG) {
      Serial.println(">>SENDING STATE!");
      addtoSendQueue("POISONING! ", "Info");
      delay(waitTime);

    }
    failsafe = true;
    for (int i = 0; i < POISONING_NUMBER; i++) {
      averageArray[arrayCount] = 160;
      arrayCount++;
      if (arrayCount == arrayLen - 1) {
        arrayCount = 0;
        if (DEBUG) {
          Serial.println(">>SENDING STATE!");
          addtoSendQueue("RESET ARRAY TO ZERO ", "Info");
          delay(waitTime);

        }
      }
    }
  } else {
    //delay(5);
    if (camThread.shouldRun()) {
      camThread.run();
      if (DEBUG) {
        Serial.println(">>SENDING STATE!");
        addtoSendQueue("RUNNING CAM THREAD! ", "Info");
        delay(waitTime);

      }
    }

    //Serial.println(xValue);
    averageArray[arrayCount] = xValue;
    long sum = 0;
    for (int i = 0; i <= arrayLen - 1; i++) {
      sum += averageArray[i];
    }

    average = sum / arrayLen;
    arrayCount++;
    if (DEBUG) {
      Serial.println(">>SENDING STATE!");
      addtoSendQueue("NEW AVERAGE: ", "Info");
      addtoSendQueue(String(average), "Info");
      delay(waitTime);

    }
  }
  // BTPrintln(average);
  if (average < 160) { // Left side of screen, turn left
    int leftAngle = map(average, 40, 159, 170, 90); // 150
//    totalSpeed = map(leftAngle, 150, 90, 70, 120);
    totalSpeed = map(leftAngle, 90, 170, 100, 50);

    directionServo.write(leftAngle);
    if (DEBUG) {
      Serial.println(">>SENDING STATE!");
      addtoSendQueue("WRITE LEFT ANGLE: ", "Info");
      addtoSendQueue(String(leftAngle), "Info");
      delay(waitTime);

    }
  } else if (average > 160) {  // Right side of screen, turn right
    int rightAngle = map(average, 161, 240, 90, 30); // 30
//    totalSpeed = map(rightAngle, 30, 90, 70, 120);
    totalSpeed = map(rightAngle, 10, 90, 50, 100);

    directionServo.write(rightAngle);
    if (DEBUG) {
      Serial.println(">>SENDING STATE!");
      addtoSendQueue("WRITE RIGHT ANGLE: ", "Info");
      addtoSendQueue(String(rightAngle), "Info");
      delay(waitTime);

    }
  } else {  // Middle of screen
    directionServo.write(90);
    if (DEBUG) {
      Serial.println(">>SENDING STATE!");
      addtoSendQueue("WRITE CENTER ", "Info");
      delay(waitTime);

    }
  }
  if (arrayCount == arrayLen - 1) {
    arrayCount = 0;
    if (DEBUG) {
      Serial.println(">>SENDING STATE!");
      addtoSendQueue("RESET ARRAY! ", "Info");
      delay(waitTime);


    }
  }
  if (DEBUG) {
    Serial.println(">>SENDING STATE!");
    addtoSendQueue("WRITE SPEED: ", "Info");
    addtoSendQueue(String(totalSpeed), "Info");
    delay(waitTime);

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
/*
   Callibrating servo code.

   Callibrates servos and then turns off the servo.

*/

void calibrateSpeedServo() {
  speedServo.write(0);
  delay(1000);
  for (int deg = 0; deg <= 180; deg += 20) {
    speedServo.write(deg);
    delay(10);
  }
  delay(2000);
  speedServo.write(0);
  return;
}
