#include <ArduinoBLE.h>
#include "Data.h"
#include "Batt.h"
#include "Display.h"
#include "pins_config.h"

static void battData(BLEDevice device, BLECharacteristic characteristic) {
  uint8_t const *data = characteristic.value();
  int const datalen = characteristic.valueLength();
  Serial.print("BATT "); Serial.print(data[0]);Serial.print(" len "); Serial.println(datalen);
  battSend(data[0]);
}

void battInit(BLEDevice *peripheral) {
  BLECharacteristic characteristic = peripheral->characteristic("2a19");
  if (characteristic) {
    Serial.println("Characteristic for battery present");
    uint8_t data[8] = {};
    if (characteristic.readValue(data, sizeof(data))) {
      int const datalen = characteristic.valueLength();
      Serial.print("BATT "); Serial.print(data[0]);Serial.print(" len "); Serial.println(datalen);
      battSend(data[0]);
    }
    if (characteristic.canSubscribe()) {
      characteristic.setEventHandler(BLEUpdated, battData);
      characteristic.subscribe();
      Serial.println("subscribed to 2A19");
    }
  } else {
    Serial.println("Battery characteristic is not present");
  }
}