#pragma once

#include <Arduino.h>
#include "wifi_comms.h"

// WiFi access point the phone/laptop (and the rover) joins to open the UI at http://192.168.4.1
// name/password live in common/wifi_comms.h so the rover uses the same ones
#define WEB_AP_SSID   WFC_WIFI_SSID
#define WEB_AP_PASS   WFC_WIFI_PASS

// Mount LittleFS (UI files), start the WiFi access point, register the pages and start the HTTP server.
// Call once in setup(), after Serial.begin() (it prints status).
void web_server_init();

// Handle any waiting HTTP request (page, tile, /api/gps, /api/rover, /api/mission). Call every loop(), returns quickly.
void web_server_update();
