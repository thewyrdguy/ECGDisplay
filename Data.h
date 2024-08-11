#pragma once

extern void dataSend(uint16_t hr, uint16_t energy, int rssi, int num, int8_t *samples);
extern uint16_t getHR(void);
extern int getRSSI(void);
extern int8_t *getSamples(int num);