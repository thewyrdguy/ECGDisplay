#include <ArduinoBLE.h>
#include <TimerEvent.h>
#include "Display.h"

#define FPS 25

#define F_HR16 0x01
#define F_HAS_ENERGY 0x08
#define F_HAS_RRI 0x10

TimerEvent updateEvent;
int failures = 0;

uint16_t hr;

void updateFrame() {
  displayFrame(hr);
}

void hrmData(BLEDevice device, BLECharacteristic characteristic) {
#if 0
  Serial.print("value updated: ");
  Serial.print("value len ");
  Serial.print(characteristic.valueLength());
  Serial.print(" : ");
  uint8_t const *p = characteristic.value();
  for (int i=0; i < characteristic.valueLength(); i++) {
    Serial.print(" ");
    Serial.print(p[i], HEX);
  }
  Serial.println();
#endif
  uint8_t const *data = characteristic.value();
  int const datalen = characteristic.valueLength();
  uint8_t const *end = data + datalen;
  uint16_t energy;
  uint16_t rri[10];
  int rris;
  uint8_t flags = *(data++);

  if (flags & F_HR16) {
    hr = data[0] + (data[1] << 8);
    data += 2;
  } else {
    hr = data[0];
    data++;
  }
  if (flags & F_HAS_ENERGY) {
    energy = data[0] + (data[1] << 8);
    data += 2;
  } else {
    energy = 0;
  }
  if (flags & F_HAS_RRI) {
    int i;
    rris = (end - data) / 2;
    if (rris > sizeof(rri) / sizeof(rri[0]))
      rris = sizeof(rri) / sizeof(rri[0]);
    for (i = 0; i < rris; i++) {
      rri[i] = data[0] + (data[1] << 8);
      data += 2;
    }
  } else {
    rris = 0;
  }
  Serial.print("HR: ");
  Serial.print(hr);
  if (energy) {
    Serial.print(", Energy: ");
    Serial.print(energy);
  }
  Serial.print(", RR Intervals:");
  for (int i = 0; i < rris; i++) {
    Serial.print(" ");
    Serial.print(rri[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(921600);
  while (!Serial);
  if (!BLE.begin()) {
    Serial.println("Staring BLE failed");
    while (1);
  }
  Serial.println("BLE Central for chest strap HRM");
  displayInit();
  updateEvent.set(1000/FPS, updateFrame);
}

void loop() {
  BLEDevice peripheral;
  BLECharacteristic characteristic;

  BLE.scanForUuid("180d");
  peripheral = BLE.available();
  if (peripheral) {
    Serial.print("Found ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();
    BLE.stopScan();
    if (peripheral.connect()) {
      Serial.println("Connected, discovering service ...");
      if (peripheral.discoverService("180d")) {
        Serial.println("Serivce present");
        characteristic = peripheral.characteristic("2a37");
        if (characteristic) {
          Serial.println("Characteristic present");
          if (characteristic.canSubscribe()) {
            characteristic.setEventHandler(BLEUpdated, hrmData);
            characteristic.subscribe();
            Serial.println("subscribed to 2a37");
            failures = 0;
          } else {
            Serial.println("Not subscribable");
            peripheral.disconnect();
            return;
          }
        } else {
          Serial.println("No characteristic");
          peripheral.disconnect();
          return;
        }
      } else {
        Serial.println("No service");
        peripheral.disconnect();
        return;
      }
    } else {
      Serial.println("Failed to connect!");
      return;
    }
  } else {
    Serial.println("Peripheral not available");
    if (failures++ > 10) {
      Serial.println("Switching off to save power");
      displayOff();
      esp_deep_sleep_start();
    }
    delay(5000);
    return;
  }
  while (peripheral.connected()) {
    BLE.poll();
    updateEvent.update();
    displayTimerHandler();
  }
  Serial.println("No longer connected");
  peripheral.disconnect();
  Serial.println("Dropped off the loop");
}
