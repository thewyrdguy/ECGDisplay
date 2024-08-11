#include "Display.h"
#include "rm67162.h"
#include <TFT_eSPI.h>
#include "pins_config.h"
#include "Data.h"

#if ARDUINO_USB_CDC_ON_BOOT != 1
#warning "If you need to monitor printed data, be sure to set USB CDC On boot to ENABLE, otherwise you will not see any data in the serial monitor"
#endif

#ifndef BOARD_HAS_PSRAM
#error "Detected that PSRAM is not turned on. Please set PSRAM to OPI PSRAM in ArduinoIDE"
#endif

#define WIDTH  536  // TFT_WIDTH
#define HEIGHT 240  // TFT_HEIGHT
#define FONTNO 4
#define WWIDTH 450
#define TFT_DARK 0x4208  // RGB 64,64,64

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
TFT_eSprite nspr = TFT_eSprite(&tft);
TFT_eSprite rspr = TFT_eSprite(&tft);

void displayInit() {
  char *hello[] = {"The Wyrd Guy's", "Contraptions", NULL};
  char url[] = "https://www.github.com/thewyrdguy/";

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH); // Power up AMOLED
  rm67162_init(); // amoled lcd initialization
  lcd_setRotation(1);
  spr.createSprite(WIDTH, HEIGHT);
  spr.setSwapBytes(1);

  spr.fillSprite(TFT_NAVY);
  spr.setTextColor(TFT_LIGHTGREY);
  spr.setTextSize(2);
  spr.setTextFont(FONTNO);
  spr.setTextDatum(4);  // Middle
  int vstep = spr.fontHeight(FONTNO);
  int vpos = spr.height() / 2 - vstep;
  for (int i = 0; hello[i]; i++) {
    spr.drawString(hello[i], spr.width() / 2, vpos, 4);
    vpos += vstep;
  }
  spr.setTextSize(1);
  spr.drawString(url, spr.width() / 2, vpos, 4);

  nspr.createSprite(spr.textWidth("555", FONTNO), spr.fontHeight(FONTNO));
  nspr.setSwapBytes(1);
  nspr.setTextColor(TFT_YELLOW);
  nspr.setTextSize(1);
  nspr.setTextFont(FONTNO);
  nspr.setTextDatum(4);

  rspr.createSprite(40, 15);
  rspr.setSwapBytes(1);

  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)spr.getPointer());
}

void displayOff(void) {
  char *farewell = "No HRM, power off";

  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(TFT_RED);
  spr.setTextSize(2);
  spr.setTextFont(FONTNO);
  spr.drawString(farewell, spr.width() / 2, spr.height() / 2, 4);
  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)spr.getPointer());
  delay(2000);
  lcd_sleep();
  digitalWrite(PIN_LED, LOW);
}

void displayStart(void) {
  spr.fillSprite(TFT_BLACK);
  spr.drawSmoothRoundRect(0, 0, 8, 5, WWIDTH + 8, spr.height() - 1, TFT_NAVY, TFT_BLACK);
  spr.drawSmoothRoundRect(WWIDTH + 13, 0, 8, 5, WIDTH - WWIDTH - 13, spr.height() - 1, TFT_DARK, TFT_BLACK);
  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)spr.getPointer());
}

void displayConn(void) {
  char *lost = "Trying to connect";

  spr.fillSprite(TFT_BLACK);
  spr.setTextColor(TFT_RED);
  spr.setTextSize(2);
  spr.setTextFont(FONTNO);
  spr.drawString(lost, spr.width() / 2, spr.height() / 2, 4);
  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)spr.getPointer());
}

static void displayHR(uint16_t hr) {
  nspr.fillSprite(TFT_BLACK);
  nspr.drawNumber(hr, nspr.width() / 2, nspr.height() / 2);
  lcd_PushColors((WWIDTH + WIDTH + 14 - nspr.width()) / 2, 60, nspr.width(), nspr.height(), (uint16_t *)nspr.getPointer());
}

static void displayRSSI(int rssi) {
  uint16_t colour = rssi >= 1 ? TFT_GREEN : TFT_RED;

  for (int i = 0; i < 5; i++) {
    int h = (i + 1) * 3;
    rspr.fillRect(i * 8, 15 - h, 5, h, i <= rssi ? colour : TFT_DARK);
  }
  lcd_PushColors(WWIDTH + 35, 20, rspr.width(), rspr.height(), (uint16_t *)rspr.getPointer());
}

static uint16_t oldhr = 0;
static int oldrssi = 0;

void displayFrame(unsigned long ms) {
  uint16_t hr = getHR();
  int rssi = getRSSI();
  if (hr != oldhr) {
    Serial.print("HR change: ");
    Serial.println(hr);
    displayHR(hr);
    oldhr = hr;
  }
  if (rssi != oldrssi) {
    Serial.print("RSSI change: ");
    Serial.println(rssi);
    displayRSSI(rssi);
    oldrssi = rssi;
  }
}