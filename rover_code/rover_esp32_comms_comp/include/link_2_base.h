#pragma once

#include <Arduino.h>
#include "wifi_comms.h"

// How often the rover sends a rover_telemetry_t to the base station
#define L2B_SEND_PERIOD_MS   1000   // GPS only updates once a second, faster gains nothing for now

// Join the base station's WiFi (WFC_WIFI_SSID) as a client and open the UDP port.
// Call once in setup(), after Serial.begin() (it prints status). Doesn't wait for the connection.
void l2b_init();

// Prints when the WiFi connects/drops, and sends telemetry every L2B_SEND_PERIOD_MS while connected.
// Call every loop(), returns quickly.
void l2b_update();
