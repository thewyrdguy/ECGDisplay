#include <ArduinoBLE.h>
#include "Data.h"
#include "norm_rssi.h"
#include "PC80B.h"
#include "Display.h"
#include "crc8.h"
#include "pins_config.h"

static void sendCmd(BLEDevice device, uint8_t cmd, uint8_t len, uint8_t *ptr) {
  uint8_t buf[64];
  assert(len + 4 < sizeof(buf));
  buf[0] = 0xa5;
  buf[1] = cmd;
  buf[2] = len;
  memcpy(buf + 3, ptr, len);
  buf[3 + len] = crc8(buf, 3 + len);

  BLECharacteristic characteristic = device.characteristic("fff2");
  if (characteristic && characteristic.canWrite()) {
    characteristic.writeValue(buf, len + 4);
    Serial.print("Written cmd");
    for (int i = 0; i < len + 4; i++) {Serial.print(" "); Serial.print(buf[i], HEX);}
    Serial.println();
  } else {
    Serial.println("No writable characteristic fff2");
  }
}

void processContinuousECG(BLEDevice device, uint8_t len, uint8_t *ptr) {
  struct cdframe {
    uint8_t seq;
    uint8_t data[50];
    uint8_t hr;
    uint8_t vol_l;
    uint8_t vol_h:4;
    uint8_t gain:3;
    uint8_t leadoff:1;
  } __attribute__((packed)) *d = (struct cdframe *)ptr;

  if ((len == 1) || (d->seq % 64 == 0)) {  // Stop data, need ACK, or every 64 frames
    uint8_t ack[2] = {d->seq, 0x00};
    sendCmd(device, 0xaa, 2, ack);
    return;
  }

  uint16_t vol = (d->vol_h << 8) + d->vol_l;
  int nsamp = 25;
  int8_t samps[25];
  for (int i = 0; i < nsamp; i++) {
    samps[i] = ((d->data[i * 2] + (d->data[i * 2 + 1] << 8)) - 2048) / 4;
  }
  dataSend(
    (struct dataset){
      .volume = vol, .gain = d->gain, .leadoff = d->leadoff, .heartrate = d->hr, .rssi = norm_rssi(device.rssi())
    },
    nsamp, samps
  );
}

void processTransferMode(BLEDevice device, uint8_t len, uint8_t *ptr) {
  struct mframe {
    uint8_t devtyp;
    uint8_t transtype:1;
    uint8_t pad1:6;
    uint8_t filtermode:1;
    uint8_t sn[12];
  } *d = (struct mframe *)ptr;
  Serial.print("Transfer ready, filtermode ");
  Serial.print(d->filtermode);
  Serial.print(" transtype ");
  Serial.print(d->transtype);
  Serial.print(" S/N ");
  for (int i = 0; i < sizeof(d->sn); i++) {
    Serial.print(d->sn[i], HEX);
  }
  Serial.println();
  uint8_t ack = (d->transtype ? 0x01 : 0x00);
  sendCmd(device, 0x55, 1, &ack);
}

void processFastECG(BLEDevice device, uint8_t len, uint8_t *ptr) {
  struct fdframe {
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
    uint8_t data[50];
  } __attribute__((packed)) *d = (struct fdframe *)ptr;
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
  dataSend(
    (struct dataset){
      .gain = d->gain, .mstage = (enum mstage_e)d->mstage,
      .mmode = (enum mmode_e)d->mmode, .channel = (enum channel_e)d->channel,
      .datatype = (enum datatype_e)d->datatype, .leadoff = d->leadoff,
      .heartrate = d->hr, .rssi = norm_rssi(device.rssi())
    },
    nsamp, samps
  );
}

void processTime(BLEDevice device, uint8_t len, uint8_t *ptr) {
  struct tmframe {
    uint8_t sc;
    uint8_t mn;
    uint8_t hr;
    uint8_t dy;
    uint8_t mo;
    uint8_t yr_l;
    uint8_t yr_h;
    uint8_t to;
  } __attribute__((packed)) *d = (struct tmframe *)ptr;
  uint16_t yr = (d->yr_h << 8) + d->yr_l;
  Serial.print("Time ");
  Serial.print(yr);
  Serial.print("-");
  Serial.print(d->mo);
  Serial.print("-");
  Serial.print(d->dy);
  Serial.print(" ");
  Serial.print(d->hr);
  Serial.print(":");
  Serial.print(d->mn);
  Serial.print(":");
  Serial.print(d->sc);
  Serial.print(" ");
  Serial.print(d->to);
  Serial.println();
}

void processDevinfo(BLEDevice device, uint8_t len, uint8_t *ptr) {
  Serial.print("DevInfo:");
  for (int i = 0; i < len; i++) {
    Serial.print(" ");
    Serial.print(ptr[i], HEX);
  }
  Serial.println();
}

void processHeartbeat(BLEDevice device, uint8_t len, uint8_t *ptr) {
  Serial.print("Heartbeat:");
  // Single byte with battery level
  for (int i = 0; i < len; i++) {
    Serial.print(" ");
    Serial.print(ptr[i], HEX);
  }
  Serial.println();
  rbattSend((ptr[0] * 30) + 10);
}

void processFrame(BLEDevice device, uint8_t *frame) {
  Serial.print(frame[2]); Serial.print(": "); Serial.print(frame[0], HEX); Serial.print(" "); Serial.print(frame[1], HEX);
  Serial.print(" "); Serial.println(frame[2]);
}

#define DEVINFO_DATA 0x11
#define TIME_DATA 0x33
#define TRANSFER_MODE_DATA 0x55
#define CONTINUOUS_ECG_DATA 0xaa
#define FAST_ECG_DATA 0xdd
#define HEARTBEAT_DATA 0xff  // contains battery level

static bool first = true;
static unsigned long next_time;
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
        case DEVINFO_DATA:
          processDevinfo(device, frame[rptr + 2], frame + rptr + 3);
          break;
        case TIME_DATA:
          processTime(device, frame[rptr + 2], frame + rptr + 3);
          break;
        case TRANSFER_MODE_DATA:
          processTransferMode(device, frame[rptr + 2], frame + rptr + 3);
          break;
        case CONTINUOUS_ECG_DATA:
          processContinuousECG(device, frame[rptr + 2], frame + rptr + 3);
          break;
        case FAST_ECG_DATA:
          processFastECG(device, frame[rptr + 2], frame + rptr + 3);
          break;
        case HEARTBEAT_DATA:
          processHeartbeat(device, frame[rptr + 2], frame + rptr + 3);
          break;
        default:
          processFrame(device, frame + rptr);
      }
      if (first) {
        sendCmd(device, 0x11, 6, (uint8_t *)"\0\0\0\0\0\0");
        first = false;
      }
      unsigned long now = millis();
      if (now > next_time) {
        sendCmd(device, 0xff, 1, (uint8_t *)"\0");
        next_time = now + 5000;
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
  next_time = millis() + 5000;
  characteristic.setEventHandler(BLEUpdated, pc80bData);
  characteristic.subscribe();
  Serial.println("Subscribed to fff1");
  return true;
}