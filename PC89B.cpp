#include <ArduinoBLE.h>
#include "Data.h"
#include "PC80B.h"
#include "Display.h"
#include "pins_config.h"

static void pc80bData(BLEDevice device, BLECharacteristic characteristic) {
  uint8_t const *val = characteristic.value();
  int vlen = characteristic.valueLength();

  Serial.print("value updated, len ");
  Serial.print(vlen);
  Serial.print(" : ");
  for (int i=0; i < vlen; i++) {
    Serial.print(" ");
    Serial.print(val[i], HEX);
  }
  Serial.println();
}

bool pc80bInit(BLEDevice *peripheral) {
  if (!peripheral->discoverAttributes()) {
    Serial.println("Characteristic discovery failed");
    return false;
  }
  BLECharacteristic characteristic = peripheral->characteristic("fff1");
  if (!characteristic) {
    Serial.println("No characteristic fff1");
    return false;
  }
  if (!characteristic.canSubscribe()) {
    Serial.println("Characteristic fff1 not subscribable");
    return false;
  }
  characteristic.setEventHandler(BLEUpdated, pc80bData);
  characteristic.subscribe();
  Serial.println("Subscribed to fff1");
  return true;
}