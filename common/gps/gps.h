#pragma once

#include <Arduino.h>
#include <TinyGPSPlus.h>

#include "wifi_comms.h"   // gps_state_t

// Shared by the base station & rover (common/gps, platformio.ini: lib_extra_dirs = ../../common)
// Each board's main.cpp passes its own pins to gps_init()
#define GPS_BASE_RX_PIN   16   // ESP32 pin that receives (wire to GPS TX)
#define GPS_BASE_TX_PIN   17   // ESP32 pin that transmits (wire to GPS RX)
#define GPS_ROVER_RX_PIN  16
#define GPS_ROVER_TX_PIN  17
#define GPS_BAUD          9600

// lattitude/longitude are double, float only has ~7 significant digits
typedef struct {
  double lat;
  double lon;
} geo_point_t;

// Start the GPS UART (Serial2)
// also starts the ring buffer for the RX interrupt
void gps_init(int rx_pin, int tx_pin);

// Drains the ring buffer & feeds the bytes into parser to extract data
void gps_update();

// GPS status as one tagged line (no newline). Goes out over USB (both boards) and WiFi (base station, GET /api/gps):
//   GPS,NODATA                          -> no bytes from the GPS (wiring / baud)
//   GPS,NOFIX,<sats>                    -> GPS talking, no position yet
//   GPS,FIX,<lat>,<lon>,<sats>,<hdop>   -> valid position
String gps_status_line();

// Same info as gps_status_line(), as numbers instead of text
typedef struct {
  gps_state_t state;   // GPS_NODATA / GPS_NOFIX / GPS_FIX
  uint8_t     sats;
  float       hdop;
  double      lat;     // only valid when state == GPS_FIX
  double      lon;
} gps_reading_t;

// Fills *out with the latest decoded GPS values
void gps_get_reading(gps_reading_t* out);

