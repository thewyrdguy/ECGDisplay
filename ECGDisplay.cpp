#include <stdbool.h>
#include <ArduinoBLE.h>
#include <TimerEvent.h>
#include "Data.h"
#include "Display.h"
#include "HRM.h"
#include "pins_config.h"

#define FPS 25

static TimerEvent updateEvent;

void updateFrame() {
  displayFrame(millis());
}

void setup() {
  Serial.begin(921600);
  // while (!Serial);
  if (!BLE.begin()) {
    Serial.println("Staring BLE failed");
    while (1);
  }
  Serial.println("BLE Central for chest strap HRM");
  displayInit();
  updateEvent.set(1000/FPS, updateFrame);
}

static BLEDevice peripheral;

void loop() {
  while (!peripheral.connected()) {
    Serial.println("Looking for the sensor");
    for (int retries = 10; ; retries--) {
      hrmInit(&peripheral);
      Serial.print("Checking after hrmInit, peripheral is ");
      Serial.println(peripheral.connected() ? "connected" : "not connected");
      if (peripheral.connected()) {
        displayStart();
        break;
      }
      if (retries) {
       Serial.print("No sensor, remaining attempts: ");
        Serial.println(retries);
        delay(5000);
        displayConn();
      } else {
        Serial.println("Switching off to save power");
        displayOff();
        esp_deep_sleep_start();
      }
    }
  }
  // Serial.println("Connected to the sensor, staring poll");
  BLE.poll();
  updateEvent.update();
}
