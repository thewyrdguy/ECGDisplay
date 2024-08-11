#include <cstdint>
#include <mutex>
#include "Data.h"

static std::mutex mtx;

uint16_t hr = 0;
uint16_t energy = 0;
int rssi = 0;
int batt = 0;

void dataSend(uint16_t p_hr, uint16_t p_energy, int p_rssi, int rris, uint16_t *rri) {
  hr = p_hr;
  energy = p_energy;
  rssi = p_rssi;
}
extern uint16_t getHR(void) {
  return hr;
}

extern int getRSSI(void) {
  return rssi;
}