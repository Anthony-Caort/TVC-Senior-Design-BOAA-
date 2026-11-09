#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <ArduinoJson.h>

#define GPS_RX_PIN  16   // ESP32 pin that receives (wire to GPS TX)
#define GPS_TX_PIN  17   // ESP32 pin that transmits (wire to GPS RX)
#define GPS_BAUD    9600

#define MAX_WAYPOINTS      50     // max waypoints we store from one mission
#define MAX_BOUNDARY_PTS   50     // max boundary vertices we store
#define MISSION_LINE_MAX   8192   // max length of one mission JSON line from the UI

TinyGPSPlus gps;
HardwareSerial GPSserial(2);

// ---------- Mission storage ----------
// lat/lon are double, float only has ~7 significant digits (about 1 m error here)
struct GeoPoint {
  double lat;
  double lon;
};

GeoPoint missionWaypoints[MAX_WAYPOINTS];
int      missionWaypointCount = 0;
GeoPoint missionBoundary[MAX_BOUNDARY_PTS];
int      missionBoundaryCount = 0;

// Characters from the UI collect here until a '\n' arrives (one mission = one line)
char   missionLineBuf[MISSION_LINE_MAX];
size_t missionLineLen = 0;
bool   missionLineOverflow = false;

// Parse one mission JSON line from the UI, store it, and reply ACK/ERR over USB
void handleMissionLine(const char* line) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) {
    Serial.printf("ERR bad JSON: %s\n", err.c_str());
    return;
  }

  JsonArray waypoints = doc["waypoints"];
  JsonArray boundary  = doc["boundary"];
  if (waypoints.isNull()) {
    Serial.println("ERR no waypoints array");
    return;
  }
  if (waypoints.size() > MAX_WAYPOINTS || boundary.size() > MAX_BOUNDARY_PTS) {
    Serial.printf("ERR too many points (max %d waypoints, %d boundary)\n", MAX_WAYPOINTS, MAX_BOUNDARY_PTS);
    return;
  }

  missionWaypointCount = 0;
  for (JsonObject wp : waypoints) {
    missionWaypoints[missionWaypointCount].lat = wp["lat"];
    missionWaypoints[missionWaypointCount].lon = wp["lon"];
    missionWaypointCount++;
  }

  missionBoundaryCount = 0;
  for (JsonObject pt : boundary) {
    missionBoundary[missionBoundaryCount].lat = pt["lat"];
    missionBoundary[missionBoundaryCount].lon = pt["lon"];
    missionBoundaryCount++;
  }

  Serial.printf("ACK %d waypoints, %d boundary pts\n", missionWaypointCount, missionBoundaryCount);
  for (int i = 0; i < missionWaypointCount; i++) {
    Serial.printf("  WP%d lat=%.7f lon=%.7f\n", i + 1, missionWaypoints[i].lat, missionWaypoints[i].lon);
  }

  // TODO: forward mission to rover over radio
}

// Non-blocking: grab whatever USB serial bytes arrived, run handleMissionLine on each full line.
// (readStringUntil would block up to 1 s and the GPS UART buffer could overflow meanwhile)
void pollMissionSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      if (missionLineOverflow) {
        Serial.printf("ERR mission too long (max %d bytes)\n", MISSION_LINE_MAX - 1);
      } else if (missionLineLen > 0) {
        missionLineBuf[missionLineLen] = '\0';
        handleMissionLine(missionLineBuf);
      }
      missionLineLen = 0;
      missionLineOverflow = false;
      continue;
    }

    if (missionLineLen < MISSION_LINE_MAX - 1) {
      missionLineBuf[missionLineLen++] = c;
    } else {
      missionLineOverflow = true;   // keep reading until '\n', then report the error
    }
  }
}

void setup() {
  Serial.setRxBufferSize(4096);   // default is 256 bytes, a mission can be a few KB. Must be before begin()
  Serial.begin(115200);
  GPSserial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS starting...");
}

void loop() {
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

  pollMissionSerial();

  // GPS status once a second, as one tagged line the UI parses into its telemetry box:
  //   GPS,NODATA                          -> no bytes from the GPS (wiring / baud)
  //   GPS,NOFIX,<sats>                    -> GPS talking, no position yet
  //   GPS,FIX,<lat>,<lon>,<sats>,<hdop>   -> valid position
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();

    if (gps.charsProcessed() < 10) {
      Serial.println("GPS,NODATA");
    } else if (gps.location.isValid()) {
      Serial.printf("GPS,FIX,%.7f,%.7f,%u,%.1f\n",
                    gps.location.lat(), gps.location.lng(),
                    gps.satellites.value(), gps.hdop.hdop());
    } else {
      Serial.printf("GPS,NOFIX,%u\n", gps.satellites.value());
    }
  }
}
