#include <stdbool.h>
#include <ArduinoBLE.h>
#include <TimerEvent.h>
#include "Data.h"
#include "Display.h"
#include "HRM.h"
#include "Batt.h"
#include "pins_config.h"

static unsigned long idle_since;

static TimerEvent battReadEvent;
static TimerEvent updateEvent;

void readBatt(void) {
  uint16_t batt = analogRead(PIN_BAT_VOLT);
  int nbat = (batt * 1000 / 6206) - 300;
  if (nbat < 0) nbat = 0;
  if (nbat > 100) nbat = 100;
  Serial.print("Local battery (raw) "); Serial.print(batt); Serial.print(" norm "); Serial.println(nbat);
  // int vref = 1100;
  // esp_adc_cal_characteristics_t adc_chars;
  // esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  // if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
  //    vref = adc_chars.vref;
  // }
  // float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
  lbatSend(nbat);
  // Use the opportunity to check disconnected time
  if (idle_since && millis() - idle_since > 50000) {
    Serial.println("Switching off to save power");
    displayOff();
    esp_deep_sleep_start();
  }
}

void updateFrame(void) {
  displayFrame(millis());
}

void discHandler(BLEDevice dev) {
  displayConn();
  BLE.scan(false);
  idle_since = millis();
  Serial.println("Disconnected, looking for the sensor");
}

void advHandler(BLEDevice dev) {
  Serial.print("Found ");
  Serial.print(dev.address());
  Serial.print(" '");
  Serial.print(dev.localName());
  Serial.print("' ");
  Serial.print(dev.advertisedServiceUuid());
  Serial.println();
  if (dev.advertisedServiceUuid() == "180d") {
    Serial.print("Found HRM sensor... ");
    BLE.stopScan();
    if (dev.connect() && dev.connected()) {
      Serial.println("connected");
      if (hrmInit(&dev)) {
        battInit(&dev);
        displayStart();
        idle_since = 0UL;
      } else {
        Serial.println("Could not intialize, disconnecting");
        // dev.disconnect();
      }
    } else {
      Serial.println("Could not connect");
    }
  }
}

void setup(void) {
  Serial.begin(921600);
  // while (!Serial);
  if (!BLE.begin()) {
    Serial.println("Staring BLE failed");
    while (1);
  }
  Serial.println("BLE Central for chest strap HRM");
  displayInit();
  battReadEvent.set(10000, readBatt);
  updateEvent.set(1000/FPS, updateFrame);
  // Initiate discovery
  BLE.setEventHandler(BLEDiscovered, advHandler);
  BLE.setEventHandler(BLEDisconnected, discHandler);
  BLE.scan(false);
  idle_since = millis();
  Serial.println("Looking for the sensor");
}

void loop(void) {
  BLE.poll();
  battReadEvent.update();
  updateEvent.update();
}