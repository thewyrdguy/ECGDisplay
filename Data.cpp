#include <Arduino.h>
#include <cstdbool>
#include <cstdint>
#include <cstring>
#include <mutex>
#include "Data.h"

static std::mutex mtx;

static struct dataset ds = {};

#define BUFSIZE 384  // 2.7 seconds worth of data at 150 SPS
static int8_t samples[BUFSIZE] = {};  // +/- 127 should be enough for display that is 240 px high
static uint16_t rdp = 0;  // we expect 25 times more reads than writes, so use read point for the base
static uint16_t amount = 0;

void dataSend(struct dataset p_ds, int p_num, int8_t *p_samples) {
  int wrp, avail, rest, buf_left;

  std::lock_guard<std::mutex> lck(mtx);

  memcpy(&ds, &p_ds, DYNSIZE);

  wrp = (rdp + amount) % BUFSIZE;
  avail = BUFSIZE - amount;
  buf_left = BUFSIZE - wrp;
  if (buf_left >= p_num) {
    memcpy(samples + wrp, p_samples, p_num);
  } else {
    memcpy(samples + wrp, p_samples, buf_left);
    memcpy(samples, p_samples + buf_left, p_num - buf_left);
  }
  if (p_num <= avail) {
    amount += p_num;
    ds.overrun = false;
  } else {
    Serial.print("Overrun by "); Serial.println(p_num - avail);
    amount = BUFSIZE;
    rdp = (wrp + p_num) % BUFSIZE;
    ds.overrun = true;
  }
}

void rbattSend(uint8_t p_rbatt) {
  std::lock_guard<std::mutex> lck(mtx);
  ds.rbatt = p_rbatt;
}

void lbattSend(uint8_t p_lbatt) {
  std::lock_guard<std::mutex> lck(mtx);
  ds.lbatt = p_lbatt;
}

static int repeated_underrun = 0;

extern void dataFetch(struct dataset *ds_p, int num, int8_t *samples_p) {
  int to_copy, to_repeat, buf_left;

  std::lock_guard<std::mutex> lck(mtx);

  if (num < amount) {
    to_copy = num;
    to_repeat = 0;
    if (repeated_underrun) {
      Serial.print("Underrun happened ");
      Serial.print(repeated_underrun);
      Serial.println(" time(s)");
    }
    repeated_underrun = 0;
    ds.underrun = false;
  } else {
    if (!repeated_underrun) {
      Serial.print("Underrun, only got ");
      Serial.print(to_copy);
      Serial.println(" samples");
    }
    repeated_underrun++;
    to_copy = amount;
    to_repeat = num - amount;
    ds.underrun = true;
  }
  buf_left = BUFSIZE - rdp;
  if (buf_left >= to_copy) {
    memcpy(samples_p, samples + rdp, to_copy);
  } else {
    memcpy(samples_p, samples + rdp, buf_left);
    memcpy(samples_p + buf_left, samples, to_copy - buf_left);
  }
  if (to_repeat)
    memset(samples_p + to_copy, samples[(rdp + to_copy - 1) % num], to_repeat);

  rdp = (rdp + to_copy) % BUFSIZE;
  amount -= to_copy;

  (*ds_p) = ds; // Do this after maybe updating underrun field
}