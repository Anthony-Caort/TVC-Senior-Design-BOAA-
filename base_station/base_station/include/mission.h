#pragma once

#include <Arduino.h>
#include "gps.h"   // geo_point_t

#define MISSION_MAX_WAYPOINTS      50     // max waypoints we store for one mission
#define MISSION_MAX_BOUNDARY_PTS   50     // max boundary vertices (corners) we store
#define MISSION_JSON_MAX           8192   // max length of one mission JSON message (USB line or WiFi body)

// Parse one mission JSON message from the UI and store it (replacing the previous mission).
// {"waypoints":[{"id":1,"lat":..,"lon":..},...],"boundary":[{"lat":..,"lon":..},...]}
// Returns the reply text (ACK/ERR lines, each ending in '\n'), the caller sends it over USB or WiFi.
// On ERR the previously stored mission is kept.
String mission_process_json(const char* mission_json);

// How many waypoints / boundary points the current mission has (0 = no mission yet)
int mission_waypoint_count();
int mission_boundary_count();

// Copy waypoint / boundary point number `index` (0-based) into *out.
// Returns false (and leaves *out alone) if index is out of range.
bool mission_get_waypoint(int index, geo_point_t* out);
bool mission_get_boundary_point(int index, geo_point_t* out);
