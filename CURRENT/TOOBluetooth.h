#ifndef TOOBluetooth_H
#define TOOBluetooth_H
#include "BluetoothSerial.h"
#include <Arduino.h>
#include <StaticThreadController.h>
#include <Thread.h>
#include <ThreadController.h>
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

bool BTStart(String connectToo);
bool BTPrintln(String msg);
bool BTPrint(String msg);
bool BTPrintln(float msg);
bool BTPrint(float msg);
bool BTStartWatchdog(int interval);
bool isLocked();
bool watchdogTick();
void flashWD();
char readData();
bool BTisReady();
bool BTForceSend(String msg);
#endif
