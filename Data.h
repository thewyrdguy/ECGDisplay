#pragma once

extern uint16_t hr;
extern uint16_t energy;
extern int rssi;

extern void dataSend(uint16_t hr, uint16_t energy, int rssi, int rris, uint16_t *rri);
extern uint16_t getHR(void);
extern int getRSSI(void);