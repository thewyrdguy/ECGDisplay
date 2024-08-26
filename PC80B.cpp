#include <ArduinoBLE.h>
#include "Data.h"
#include "PC80B.h"
#include "Display.h"
#include "crc8.h"
#include "pins_config.h"

typedef struct {
  uint8_t seq_l;
  uint8_t seq_h;
  uint8_t pad1:4;
  uint8_t gain:3;
  uint8_t pad2:1;
  uint8_t mstage:4;
  uint8_t mmode:2;
  uint8_t channel:2;
  uint8_t hr;
  uint8_t datatype:3;
  uint8_t pad3:4;
  uint8_t leadoff:1;
  uint8_t data[];
} __attribute__((packed)) fast_ecg_data;

void processFastECG(uint8_t len, uint8_t *ptr) {
  fast_ecg_data *d = (fast_ecg_data*)ptr;
  uint16_t seq = d->seq_l + (d->seq_h << 8);
#if 0
  Serial.print("Seq ");
  Serial.print(seq);
  Serial.print(" gain ");
  Serial.print(d->gain);
  Serial.print(" channel ");
  Serial.print(d->channel);
  Serial.print(" mmode ");
  Serial.print(d->mmode);
  Serial.print(" mstage ");
  Serial.print(d->mstage);
  Serial.print(" hr ");
  Serial.print(d->hr);
  Serial.print(" leadoff ");
  Serial.print(d->leadoff);
  Serial.print(" datatype ");
  Serial.print(d->datatype);
  Serial.print(" data elements ");
  Serial.print(len - 6);
  Serial.println();
#endif
  int nsamp = 25;
  int8_t samps[25];
  for (int i = 0; i < nsamp; i++) {
    samps[i] = ((d->data[i * 2] + (d->data[i * 2 + 1] << 8)) - 2048) / 4;
  }
  dataSend(d->hr, 0, 0, nsamp, samps);
}

void processFrame(uint8_t *frame) {
  Serial.print(frame[2]); Serial.print(": "); Serial.print(frame[0], HEX); Serial.print(" "); Serial.print(frame[1], HEX);
  Serial.print(" "); Serial.println(frame[2]);
}

#define FAST_ECG_DATA 0xdd

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
    uint8_t crc = crc8(frame + rptr, flen - 1);
    if ((frame[rptr] == 0xa5) && (crc == frame[rptr + flen - 1])) {
      switch (frame[rptr + 1]) {
        case FAST_ECG_DATA:
          processFastECG(frame[rptr + 2], frame + rptr + 3);
          break;
        default:
          processFrame(frame + rptr);
      }
    } else {
      Serial.print("Frame tag ");
      Serial.print(frame[rptr], HEX);
      Serial.print(" crc calculated ");
      Serial.print(crc, HEX);
      Serial.print(" present ");
      Serial.println(frame[rptr + flen - 1], HEX);
    }
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