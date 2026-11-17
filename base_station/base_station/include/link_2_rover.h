#pragma once

#include <Arduino.h>
#include "wifi_comms.h"

// No telemetry for this long = rover considered lost (it sends every 1 s, so ~5 missed in a row)
#define L2R_TIMEOUT_MS   5000

// Open the UDP port the rover sends to. Called once in setup(), AFTER web_server_init() (WiFi must be up).
void l2r_init();

// Read any telemetry packets that arrived, keep the newest. Call every loop(), returns quickly.
void l2r_update_rover_telemetry();

// Copies the newest telemetry into *copy_into. Returns false if nothing heard within L2R_TIMEOUT_MS.
bool l2r_get_rover_telemetry(rover_telemetry_t* copy_into);

// Rover status as one tagged line (no newline), same style as gps_status_line():
//   ROVER,NOLINK                          -> no telemetry within L2R_TIMEOUT_MS
//   ROVER,NODATA                          -> rover is talking, but its GPS isn't
//   ROVER,NOFIX,<sats>                    -> rover GPS talking, no position yet
//   ROVER,FIX,<lat>,<lon>,<sats>,<hdop>   -> valid rover position
String l2r_rover_status_line();
