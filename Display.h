#pragma once

#define SPS 150
#define FPS 25

extern void displayInit(void);
extern void displayOff(void);
extern void displayStart(void);
extern void displayConn(void);
extern void displayFrame(unsigned long ms);