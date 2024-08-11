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
static int batt = 0;

#define BUFSIZE 1024
static int8_t samples[BUFSIZE] = {};  // +/- 127 should be enough for display that is 240 px high
static uint16_t rdptr = 0;
static uint16_t wrptr = 0;

void dataSend(uint16_t p_hr, uint16_t p_energy, int p_rssi, int p_num, int8_t *p_samples) {
  int to_copy;
  bool overrun = false;

  std::lock_guard<std::mutex> lck(mtx);
  hr = p_hr;
  energy = p_energy;
  rssi = p_rssi;
  if ((rdptr > wrptr && rdptr < (wrptr + p_num)) ||
      ((rdptr + BUFSIZE) > wrptr && (rdptr + BUFSIZE) < (wrptr + p_num))) {
      overrun = true;
      Serial.print("Overrun: rdptr = ");
      Serial.print(rdptr);
      Serial.print(" wrptr = ");
      Serial.println(wrptr);
   }
  do {
    to_copy = (wrptr + p_num < BUFSIZE) ? p_num : (BUFSIZE - wrptr);
    p_num -= to_copy;
    std::memcpy(samples+wrptr, p_samples, to_copy * sizeof(samples[0]));
    wrptr += to_copy;
    if (wrptr >= BUFSIZE) wrptr = 0;
  } while (p_num);
  if (overrun) rdptr = wrptr;
}

uint16_t getHR(void) {
  std::lock_guard<std::mutex> lck(mtx);
  return hr;
}

int getRSSI(void) {
  std::lock_guard<std::mutex> lck(mtx);
  return rssi;
}

#define MAXSAMP 8  // It's 6 for 150 Hz, 25 fps, or 5 for 30 fps.
static int8_t buf[MAXSAMP] = {};

int8_t *getSamples(int num) {
  int i;
  int to_copy;

  assert(num < MAXSAMP);
  std::lock_guard<std::mutex> lck(mtx);
  if ((rdptr < wrptr && (rdptr + num) > wrptr) || 
      (rdptr < (wrptr + BUFSIZE) && (rdptr + num) > (wrptr + BUFSIZE))) {
      to_copy = (wrptr - rdptr + BUFSIZE) % BUFSIZE;
      Serial.print("Underun: rdptr = ");
      Serial.print(rdptr);
      Serial.print(" wrptr = ");
      Serial.print(wrptr);
      Serial.print(" to_copy ");
      Serial.println(to_copy);
   } else {
    to_copy = num;
   }
  for (i = 0; i < to_copy && rdptr < BUFSIZE; i++)
    buf[i] = samples[rdptr++];
  if (rdptr >= BUFSIZE)
    rdptr = 0;
  for (; i < to_copy; i++)
    buf[i] = samples[rdptr++];
  for (; i < num; i++)
    buf[i] = 0;
  return buf;
  // TODO handle buffer underrun
}