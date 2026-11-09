#pragma once

#include <Arduino.h>
#include <TinyGPSPlus.h>

#define GPS_RX_PIN  16   // ESP32 pin that receives (wire to GPS TX)
#define GPS_TX_PIN  17   // ESP32 pin that transmits (wire to GPS RX)
#define GPS_BAUD    9600

// lattitude/longitude are double, float only has ~7 significant digits
typedef struct {
  double lat;
  double lon;
} geo_point_t;

// Start the GPS UART (Serial2)
// also starts the ring buffer for the RX interrupt
void gps_init();

// Drains the ring buffer & feeds the bytes into parser to extract data
void gps_update();

// GPS status as one tagged line (no newline). Same text goes out over USB and WiFi:
//   GPS,NODATA                          -> no bytes from the GPS (wiring / baud)
//   GPS,NOFIX,<sats>                    -> GPS talking, no position yet
//   GPS,FIX,<lat>,<lon>,<sats>,<hdop>   -> valid position
String gps_status_line();

