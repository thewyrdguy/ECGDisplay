#include <ArduinoBLE.h>
#include "Data.h"
#include "HRM.h"
#include "Display.h"
#include "pins_config.h"

#define F_HR16 0x01
#define F_HAS_ENERGY 0x08
#define F_HAS_RRI 0x10

static BLECharacteristic characteristic;

#define SBUFSIZE 1024
static int8_t samples[SBUFSIZE];

static void makeSamples(int rris, uint16_t rri[], int *nump, int8_t sampp[]) {
  int wp = 0;

  for (int i = 0; i < rris; i++) {
    // RR Interval comes in 1/1024 of a second. We want SPS (150 / sec) samples.
    int num = rri[i] * SPS / 1024;  // maybe we can do better if we keep running time in 1/1024 sec...
    for (int j = 0; (j < num) && (wp < *nump); j++, wp++)
      sampp[wp] = j < 15 ? 200 : (j < 30 ? -200 : 0);
  }
  //Serial.print("Total samples ");Serial.print(wp); Serial.print(" of max ");Serial.println(*nump);
  (*nump) = wp;
}

static void hrmData(BLEDevice device, BLECharacteristic characteristic) {
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
  uint16_t hr;
  uint16_t energy;
  int rssi = 0;
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
  rssi = (device.rssi() + 100) / 10;
  if (rssi < 0) rssi = 0;
  if (rssi > 4) rssi = 4;

  Serial.print("RSSI: ");
  Serial.print(rssi);
  Serial.print(" HR: ");
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

  int num = SBUFSIZE;
  makeSamples(rris, rri, &num, samples);
  dataSend(hr, energy, rssi, num, samples);
}

void hrmInit(BLEDevice *peripheral) {
  BLE.scanForUuid("180d");
  (*peripheral) = BLE.available();
  if (*peripheral) {
    BLE.stopScan();
    Serial.print("Found ");
    Serial.print(peripheral->address());
    Serial.print(" '");
    Serial.print(peripheral->localName());
    Serial.print("' ");
    Serial.print(peripheral->advertisedServiceUuid());
    Serial.println();
    if (peripheral->connect()) {
      Serial.println("Connected, discovering service ...");
      if (peripheral->discoverService("180d")) {
        Serial.println("Serivce present");
        characteristic = peripheral->characteristic("2a37");
        if (characteristic) {
          Serial.println("Characteristic present");
          if (characteristic.canSubscribe()) {
            characteristic.setEventHandler(BLEUpdated, hrmData);
            characteristic.subscribe();
            Serial.println("subscribed to 2a37");
          } else {
            Serial.println("Not subscribable");
            peripheral->disconnect();
          }
        } else {
          Serial.println("No characteristic");
          peripheral->disconnect();
        }
      } else {
        Serial.println("No service");
        peripheral->disconnect();
      }
    } else {
      Serial.println("Failed to connect!");
    }
  }
  Serial.print("After hrmInit, peripheral is ");
  Serial.println(peripheral->connected() ? "connected" : "not connected");
}