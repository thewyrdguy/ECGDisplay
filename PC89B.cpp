#include <ArduinoBLE.h>
#include "Data.h"
#include "PC80B.h"
#include "Display.h"
#include "pins_config.h"

void processFrame(int size, uint8_t *frame) {
  Serial.print(size); Serial.print(": "); Serial.print(frame[0], HEX); Serial.print(" "); Serial.print(frame[1], HEX);
  Serial.print(" "); Serial.println(frame[2]);
}

#define BLE_MAX 512  // 512 is the max size of BLE characteristic value. In reality
static uint8_t frame[BLE_MAX] = {};
static int wptr;

static void pc80bData(BLEDevice device, BLECharacteristic characteristic) {
  uint8_t const *val = characteristic.value();
  int vlen = characteristic.valueLength();
  if (vlen + wptr > BLE_MAX) {
    Serial.print("Too much data");
    wptr = 0;
  }
  memcpy(frame + wptr, val, vlen);
#if 0
  Serial.print("New ");
  Serial.print(vlen);
  Serial.print(" bytes, frame now:");
  for (int i = 0; i < wptr + vlen; i++) {
    Serial.print(" ");
    Serial.print(frame[i], HEX);
  }
  Serial.println();
#endif
  if ((wptr == 0) && (frame[0] != 0xa5)) {  // Fresh message must start the frame
    Serial.println("Bad frame, drop");
    return;
  }
  wptr += vlen;
  int rptr;
  for (rptr = 0; rptr < wptr; ) {
    uint8_t flen = frame[rptr + 2] + 4;
    if (rptr + flen > wptr) {  // Frame incomplete, wait for more messages
      break;
    }
    processFrame(flen, frame + rptr);
    rptr += flen;
  }
  if (wptr - rptr) {
    memmove(frame, frame + rptr, wptr - rptr);
  }
  wptr -= rptr;
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
  wptr = 0;
  characteristic.setEventHandler(BLEUpdated, pc80bData);
  characteristic.subscribe();
  Serial.println("Subscribed to fff1");
  return true;
}