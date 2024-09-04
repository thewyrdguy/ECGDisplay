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

#define WIDTH 536   // TFT_WIDTH
#define HEIGHT 240  // TFT_HEIGHT
#define FONTNO 4
#define WWIDTH 450
#define FWIDTH (SPS / FPS)
#define TFT_DARK 0x4208  // RGB 64,64,64

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
TFT_eSprite nspr = TFT_eSprite(&tft);
TFT_eSprite espr = TFT_eSprite(&tft);
TFT_eSprite bspr = TFT_eSprite(&tft);

static bool animating;
char hello[] PROGMEM = "The Wyrd Things";
char url[] PROGMEM = "https://www.github.com/thewyrdguy/";
char scanning[] PROGMEM = "Searching for an HRM or PC80B sensor";
char reconnecting[] PROGMEM = "Connection lost, searching again";
char farewell[] PROGMEM = "No sensor found, switching off to save battery";

static void displayIntro(char *msg) {
  spr.fillSprite(TFT_NAVY);
  spr.setTextColor(TFT_LIGHTGREY);
  spr.setTextSize(3);
  spr.setTextFont(FONTNO);
  spr.setTextDatum(4);  // Middle
  int vstep = spr.fontHeight(FONTNO);
  int vpos = spr.height() / 2;
  spr.setTextSize(2);
  spr.drawString(hello, spr.width() / 2, vpos, 4);
  spr.setTextSize(1);
  spr.drawString(url, spr.width() / 2, vpos - vstep, 4);
  spr.setTextColor(TFT_YELLOW);
  spr.drawString(msg, spr.width() / 2, vpos + vstep, 4);
  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)spr.getPointer());
}

void displayInit() {
  animating = false;

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);  // Power up AMOLED
  rm67162_init();               // amoled lcd initialization
  lcd_setRotation(1);

  spr.createSprite(WIDTH, HEIGHT);
  spr.setSwapBytes(1);
  displayIntro(scanning);

#if 0
  Serial.print("Width of 3 chars ");
  Serial.print(spr.textWidth("555", FONTNO));
  Serial.print(" hight of line ");
  Serial.println(spr.fontHeight(FONTNO));
  // Result is w=42, h=26
#endif
  nspr.createSprite(spr.textWidth("555", FONTNO), spr.fontHeight(FONTNO));
  nspr.setSwapBytes(1);
  nspr.setTextColor(TFT_YELLOW);
  nspr.setTextSize(1);
  nspr.setTextFont(FONTNO);
  nspr.setTextDatum(4);

  //bspr.createSprite(spr.textWidth("555", FONTNO), spr.fontHeight(FONTNO));
  bspr.createSprite(40, 15);
  bspr.setSwapBytes(1);
  bspr.setTextColor(TFT_DARKGREY);
  bspr.setTextSize(1);
  bspr.setTextFont(FONTNO);
  bspr.setTextDatum(4);

  espr.createSprite(FWIDTH, HEIGHT - 10);
  espr.setSwapBytes(1);
}

void displayOff(void) {
  animating = false;
  displayIntro(farewell);
  delay(2000);
  lcd_sleep();
  digitalWrite(PIN_LED, LOW);
}

void displayStart(void) {
  spr.fillSprite(TFT_BLACK);
  spr.drawSmoothRoundRect(0, 0, 8, 5, WWIDTH + 8, spr.height() - 1, TFT_NAVY, TFT_BLACK);
  spr.drawSmoothRoundRect(WWIDTH + 13, 0, 8, 5, WIDTH - WWIDTH - 13, spr.height() - 1, TFT_NAVY, TFT_BLACK);
  lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t *)spr.getPointer());
  animating = true;
}

void displayConn(void) {
  animating = false;
  displayIntro(reconnecting);
}

static int dpos = 0;
static int oldvpos = 127;

static void displaySamples(int num, int8_t *samples) {
  for (int i = 0; i < num; i++) {
    int vpos = 120 - samples[i];
    int hcoord, height;
    if (vpos > 235) vpos = 235;
    if (vpos < 5) vpos = 5;
    if (oldvpos < vpos) {  // old is on the top
      hcoord = oldvpos;
      height = vpos - oldvpos;
    } else {
      hcoord = vpos;
      height = oldvpos - vpos;
    }
    if (height == 0) height = 1;
    espr.drawFastVLine(i, 0, espr.height(), TFT_BLACK);
    espr.drawFastVLine(i, hcoord, height, TFT_GREEN);  // on top
    oldvpos = vpos;
  }
  lcd_PushColors(dpos + 5, 5, espr.width(), espr.height(), (uint16_t *)espr.getPointer());
  dpos += num;
  if (dpos >= WWIDTH) dpos = 0;
  espr.drawFastVLine(0, 0, espr.height(), TFT_DARKGREY);
  espr.fillRect(1, 0, num - 1, espr.height(), TFT_BLACK);
  lcd_PushColors(dpos + 5, 5, espr.width(), espr.height(), (uint16_t *)espr.getPointer());
}

static void displayHR(uint16_t hr, int x, int y) {
  nspr.fillSprite(TFT_BLACK);
  if (hr) nspr.drawNumber(hr, nspr.width() / 2, nspr.height() / 2);
  lcd_PushColors(x, y, nspr.width(), nspr.height(), (uint16_t *)nspr.getPointer());
}

static void displayRSSI(int rssi, int x, int y) {
  uint16_t colour = rssi >= 1 ? TFT_GREEN : TFT_RED;

  bspr.fillSprite(TFT_BLACK);
  for (int i = 0; i < 5; i++) {
    int h = (i + 1) * 3;
    bspr.fillRect(i * 8, 15 - h, 5, h, i <= rssi ? colour : TFT_DARK);
  }
  lcd_PushColors(x, y, bspr.width(), bspr.height(), (uint16_t *)bspr.getPointer());
}

static void displayLead(bool leadoff, int x, int y) {
  bspr.fillSprite(TFT_BLACK);
  uint32_t colour = leadoff ? TFT_RED : TFT_BLUE;
  uint32_t r = bspr.height() / 2;
  bspr.fillSmoothCircle(r, r, r, colour, TFT_BLACK);
  bspr.fillSmoothCircle(bspr.width() - r - 1, r, r, colour, TFT_BLACK);
  if (leadoff) {
    uint32_t w = bspr.width() / 2;
    bspr.fillRect(1, r - 2, w - 4, 5, colour);
    bspr.fillRect(w + 3, r - 2, w - 3, 5, colour);
  } else {
    bspr.fillRect(0, r - 2, bspr.width(), 5, colour);
  }
  lcd_PushColors(x, y, bspr.width(), bspr.height(), (uint16_t *)bspr.getPointer());
}

static void displayBatt(uint8_t batt, int x, int y) {
  uint16_t colour = batt >= 15 ? TFT_DARKGREEN : TFT_RED;

  if (batt > 100) batt = 100;
  bspr.fillSprite(TFT_BLACK);
  bspr.drawRect(0, 0, bspr.width(), bspr.height(), colour);
  bspr.fillRect(0, 0, bspr.width() * batt / 100, bspr.height(), colour);
  //bspr.drawNumber(batt, bspr.width() / 2, bspr.height() / 2);
  lcd_PushColors(x, y, bspr.width(), bspr.height(), (uint16_t *)bspr.getPointer());
}

static void displayMmode(enum mmode_e mmode, int x, int y) {
  int y2 = bspr.height() / 2;
  int w = bspr.width();
  int shift;
  uint16_t colour = mmode == mm_detecting ? TFT_DARKGREY : TFT_BLUE;
  bspr.fillSprite(TFT_BLACK);
  if (mmode != mm_continuous) {
    bspr.fillRect(0, 0, 5, bspr.height(), colour);
    bspr.fillRect(bspr.width() - 5, 0, 5, bspr.height(), colour);
    shift = 5;
    bspr.fillRect(shift + 5, y2 - 2, w - shift - 5, 5, colour);
    bspr.fillTriangle(shift, y2, shift + 10, 0, shift + 10, bspr.height(), colour);
  } else {
    shift = 0;
    bspr.fillRect(0, y2 - 2, w - 5, 5, colour);
  }
  bspr.fillTriangle(w - shift, y2, w - shift - 10, 0, w - shift - 10, bspr.height(), colour);
  lcd_PushColors(x, y, bspr.width(), bspr.height(), (uint16_t *)bspr.getPointer());
}

static void displayMstage(enum mstage_e mstage, int x, int y) {
  uint16_t colour = mstage == ms_stop ? TFT_RED : TFT_BLUE;

  bspr.fillSprite(TFT_BLACK);
  for (int i = 0; i < 5; i++) {
    bspr.fillRect(i * 8, 0, 5, 6, i <= (uint8_t) mstage ? colour : TFT_DARK);
  }
  lcd_PushColors(x, y, bspr.width(), bspr.height(), (uint16_t *)bspr.getPointer());
}

static struct dataset old_ds = { .mstage = ms_stop, .leadoff = true };

#define IF_CHANGED(FL, FUNC, X, Y) \
  do { \
    if (ds.FL != old_ds.FL) { \
      Serial.print(#FL " change from "); \
      Serial.print(old_ds.FL); \
      Serial.print(" to "); \
      Serial.println(ds.FL); \
      old_ds.FL = ds.FL; \
      FUNC(ds.FL, X, Y); \
    } \
  } while (0)

void displayFrame(unsigned long ms) {
  if (!animating) return;

  struct dataset ds;
  int8_t samples[FWIDTH];
  dataFetch(&ds, FWIDTH, samples);

  displaySamples(FWIDTH, samples);
  IF_CHANGED(heartrate, displayHR, WWIDTH + 30, 80);
  IF_CHANGED(leadoff, displayLead, WWIDTH + 31, 115);
  IF_CHANGED(rssi, displayRSSI, WWIDTH + 35, 20);
  IF_CHANGED(rbatt, displayBatt, WWIDTH + 31, 50);
  IF_CHANGED(lbatt, displayBatt, WWIDTH + 31, HEIGHT - 35);
  IF_CHANGED(mmode, displayMmode, WWIDTH + 31, 135);
  IF_CHANGED(mstage, displayMstage, WWIDTH + 31, 160);
  old_ds = ds;
}