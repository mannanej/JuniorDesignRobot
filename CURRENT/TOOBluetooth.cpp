/*
   TOO Bluetooth Manager
   Zackery A. Painter
   12-16-2021
   Team TOO ECE362 - Winter 2021-22 RHIT

   The purpose of this Library is to provide a good connection using the ESP32 DEV's bluetooth radio and the BluetoothSerial Library
   This Library contains a standard startup and connection method as well as a watchdog timer to ensure we know if we've lost connection.
*/
#include <StaticThreadController.h>
#include <Thread.h>
#include <ThreadController.h>

#include "TOOBluetooth.h"
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#define TOOBluetooth
BluetoothSerial SerialBT;
boolean confirmRequestPending = true;
static bool btScanAsync = true;
 String connectTo = "";
int pairButton = 34;
bool watchdog = false;
bool lockout = false;
int watchdogTimer = 0;
int watchdogTimeout = 10;
Thread watchdogThread = Thread();

/*

   Bluetooth Helper methods / Callbacks
*/
void BTConfirmRequestCallback(uint32_t numVal)


{
  confirmRequestPending = true;
  //  Serial.print("Please verify: ");
  //  Serial.println(numVal);
}

void BTAuthCompleteCallback(boolean success)
{
  confirmRequestPending = false;
  if (success)
  {
    //   Serial.println("Pairing success!!");
  }
  else
  {
    //   Serial.println("Pairing failed, rejected by user!!");
  }
}
//Valid device flag
bool validDev = false;
/*

   Sets validDev to true if we find a valid device to connect to.
*/
void btAdvertisedDeviceFound(BTAdvertisedDevice* pDevice) {
  Serial.printf("Found a device asynchronously: %s\n", pDevice->toString().c_str());

  if (strcmp(pDevice->getName().c_str(), connectTo.c_str()) == 0) {
    Serial.println("Requested device found... Initiating connect...");
    if (!watchdog) {
      SerialBT.connect(pDevice->toString().c_str());

      SerialBT.discoverAsyncStop(); // Stop looking for devices
    }
    validDev = true;
    watchdogTimer = 0;
  } else {
    digitalWrite(2, HIGH);
    delay(50);
    digitalWrite(2, LOW);
    Serial.println("Ignoring device...");
  }
}
/*

   External method to send a message
*/
bool BTPrintln(String msg) {
  if (SerialBT.available()) {
    SerialBT.println(msg);
   // SerialBT.flush();

    return true;
  } else {
    return false;
  }
}
char readData(){
//  while(!BTisReady());
  char dat =SerialBT.read();
  return dat;
}
bool BTPrint(String msg) {
  if (SerialBT.available()) {
    SerialBT.print(msg);
    //    SerialBT.flush();
    return true;
  } else {
    return false;
  }
}
bool BTisReady(){
  return SerialBT.available();
}
bool BTPrintln(float msg) {
  if (SerialBT.available()) {
    SerialBT.println(msg);
    //  SerialBT.flush();

    return true;
  } else {
    return false;
  }
}
bool BTForceSend(String msg){
  SerialBT.print(msg);
  return true;
}
bool BTPrint(float msg) {
  if (SerialBT.available()) {
    SerialBT.print(msg);
    //SerialBT.flush();
    return true;
  } else {
    return false;
  }
}
void flashWD() {
  digitalWrite(2, HIGH);
  delay(50);
  digitalWrite(2, LOW);
  delay(50);
  digitalWrite(2, HIGH);
  delay(50);
  digitalWrite(2, LOW);
}
/*

   This is triggered each time the thread is run.
*/
void BTWatchdog() {

  if (!SerialBT.hasClient()) {
//    if(false){
    watchdogTimer++;
    if (watchdogTimer > watchdogTimeout) { // If we disconnect, lockup
      SerialBT.disconnect();
      lockout = true;
      Serial.println("WATCHDOG: WARN: DISCONNECTED! >> Enabled failure interlock.");

    }
  } else {
   // Serial.println("Triggered Thread: Watchdog");
    watchdogTimer = 0;
  }
}
/*

   Start our watchdog timer
*/
bool BTStartWatchdog(int interval) {
  //Serial.println("Starting watchdog...");
  digitalWrite(2, HIGH);
  delay(50);
  digitalWrite(2, LOW);
  watchdog = true;

  watchdogThread.enabled = true;
  watchdogThread.setInterval(interval);
  //  watchdogThread.ThreadName = "Bluetooth Watchdog";
  watchdogThread.onRun(BTWatchdog);
  watchdogThread.run();
  return true;
}
/*

   Return lockout information
*/
bool isLocked() {
  return lockout;
}/*
  Call our thread if we need to...

*/
bool watchdogTick() {
  if (watchdogThread.shouldRun()) {
    SerialBT.flush();
    watchdogThread.run();
    return true;
  } else {
    return false;
  }

}
/*

   Start Bluetooth

*/
bool BTStart(String connectToo) {
  connectTo=connectToo;
  SerialBT.enableSSP();
  SerialBT.onConfirmRequest(BTConfirmRequestCallback);
  SerialBT.onAuthComplete(BTAuthCompleteCallback);
  SerialBT.begin("Team TOO"); //Bluetooth device name
  //Serial.println("Bluetooth Started... Runing ASYNC device discovery...");
  // Start system LED for error codes
  pinMode(2, OUTPUT);
  pinMode(pairButton, INPUT);
  /*
   * Flash to indicate starting BT search...
   * 
   */
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
  delay(100);
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
  //Start Async Scan for BT devices...
  if (btScanAsync) {
    Serial.print("......"); // Show progress...
    delay(100);
    if (SerialBT.discoverAsync(btAdvertisedDeviceFound)) {
      Serial.println("..."); // Show progress...
      delay(10000); // Wait 10 seconds
      Serial.print("Stopping discoverAsync... ");
      SerialBT.discoverAsyncStop(); // Stop looking for devices
      Serial.println("stopped");
      // If our Async function set our flag correctly, we should be able to connect...
      if (validDev) {
        Serial.println("Please connect via COM port now!");
        digitalWrite(2, HIGH); // Show user it's time to connect!
        for (int i = 0; i < 310; i++) {
          delay(100);
          if (SerialBT.connected() || digitalRead(pairButton) == LOW) { // Break if we connect!
            break;
          }
        }
        while (digitalRead(pairButton) == LOW) {
          delay(10);
        }
        // If we didn't connect after 30 seconds, go into pairing mode
        bool done = false;
        if (! SerialBT.connected()) {
          Serial.println("Connection was not established! Entering pairing mode...");

          // Wait for pairing to complete
          while (! SerialBT.connected()) {

            //Slow flash
            digitalWrite(2, HIGH);
            delay(500);
            digitalWrite(2, LOW);
            delay(500);

            // Check to see if anyone is waiting to pair...
            if (! confirmRequestPending) {
              if (done) {
                break;
              } else {
                Serial.println("Waiting for pair request...");
              }
            } else {
              // If we receive a pairing request, try to authenticate (Should make button press in future)
              //int dat = Serial.read();
              int waitTime = 0;
              int timeout = 1000;

              if (digitalRead(pairButton) == LOW)
              {
                done = true;
                digitalWrite(2, LOW);
                //Send confirmation if correct
                SerialBT.confirmReply(true);
                while (digitalRead(pairButton) == LOW) {
                  delay(10);
                  Serial.println("Waiting for button release...");
                }


              }
              else
              {
                delay(100);
                //Fail if incorrect was sent...
                if (waitTime == timeout) {
                  SerialBT.confirmReply(false);
                  Serial.println("Failed... Timeout");
                }
              }
            }

          }

          //Serial.println("Connected!");

        } else {
          // If we get here, it means we connected in time and don't need to re-pair
          //Serial.println("Connection Started!");
          confirmRequestPending = false; //Disable confirmation checking
          digitalWrite(2, LOW); //Reset our LED
        }

      } else {
        // If we get here, it means that we couldn't find the device, Try rebooting or make sure the device is in range

        Serial.println("Device isn't found... Please pair device or ensure it is in range!");

        // Flash error code (Fast flash)
        while (1) {
          digitalWrite(2, HIGH);
          delay(50);
          digitalWrite(2, LOW);
          delay(50);
        }
      }
    } else {
      // Something bad happened if we got here... The discover function failed
      // Serial.println("Error on discoverAsync!");
      // Do a blink code here Long - Short- Long pause
      while (1) {
        //Long - Short - (Long pause)
        digitalWrite(2, HIGH);
        delay(1000);
        digitalWrite(2, LOW);
        delay(1000);
        digitalWrite(2, HIGH);
        delay(100);
        digitalWrite(2, LOW);
        delay(2000);


      }
    }
  }
  //delay(1000); // Wait for cooldown
  return true;
}
