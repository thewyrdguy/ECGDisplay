#include <Arduino.h>
#include <cstdbool>
#include <cstdint>
#include <cstring>
#include <mutex>
#include "Data.h"

static std::mutex mtx;

static uint16_t hr = 0;
static uint16_t energy = 0;
static int rssi = 0;
static uint8_t batt = 0;

#define BUFSIZE 384  // 2.7 seconds worth of data at 150 SPS
static int8_t samples[BUFSIZE] = {};  // +/- 127 should be enough for display that is 240 px high
static uint16_t rdp = 0;  // we expect 25 times more reads than writes, so use read point for the base
static uint16_t amount = 0;

void dataSend(uint16_t p_hr, uint16_t p_energy, int p_rssi, int p_num, int8_t *p_samples) {
  int wrp, avail, rest, buf_left;

  std::lock_guard<std::mutex> lck(mtx);
  hr = p_hr;
  energy = p_energy;
  rssi = p_rssi;

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
  } else {
    Serial.print("Overrun by "); Serial.println(p_num - avail);
    amount = BUFSIZE;
    rdp = (wrp + p_num) % BUFSIZE;
  }
}

void battSend(uint8_t p_batt) {
  std::lock_guard<std::mutex> lck(mtx);
  batt = p_batt;
}

uint16_t getHR(void) {
  std::lock_guard<std::mutex> lck(mtx);
  return hr;
}

int getRSSI(void) {
  std::lock_guard<std::mutex> lck(mtx);
  return rssi;
}

uint8_t getBatt(void) {
  std::lock_guard<std::mutex> lck(mtx);
  return batt;
}

#define MAXSAMP 8  // It's 6 for 150 Hz, 25 fps, or 5 for 30 fps.
static int8_t buf[MAXSAMP] = {};

int8_t *getSamples(int num) {
  int to_copy, to_repeat, buf_left;

  assert(num < MAXSAMP);
  std::lock_guard<std::mutex> lck(mtx);
  if (num < amount) {
    to_copy = num;
    to_repeat = 0;
  } else {
    Serial.print("Underrun, only got "); Serial.println(to_copy);
    to_copy = amount;
    to_repeat = num - amount;
  }
  buf_left = BUFSIZE - rdp;
  if (buf_left >= to_copy) {
    memcpy(buf, samples + rdp, to_copy);
  } else {
    memcpy(buf, samples + rdp, buf_left);
    memcpy(buf + buf_left, samples, to_copy - buf_left);
  }
  if (to_repeat)
    memset(buf + to_copy, samples[(rdp + to_copy - 1) % BUFSIZE], to_repeat);

  rdp = (rdp + to_copy) % BUFSIZE;
  amount -= to_copy;
  return buf;
}