#include <Arduino.h>

#include "gps.h"
#include "mission.h"
#include "web_server.h"
#include "link_2_rover.h"

// ---------- USB serial link ----------
// Characters from the UI collect here until a '\n' arrives (one mission = one line)
char   missionLineBuf[MISSION_JSON_MAX];
size_t missionLineLen = 0;
bool   missionLineOverflow = false;

// Non-blocking: grab whatever USB serial bytes arrived, run mission_process_json on each full line.
// (readStringUntil would block up to 1 s and the GPS UART buffer could overflow meanwhile)
void pollMissionSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      if (missionLineOverflow) {
        Serial.printf("ERR mission too long (max %d bytes)\n", MISSION_JSON_MAX - 1);
      } else if (missionLineLen > 0) {
        missionLineBuf[missionLineLen] = '\0';
        Serial.print(mission_process_json(missionLineBuf));
      }
      missionLineLen = 0;
      missionLineOverflow = false;
      continue;
    }

    if (missionLineLen < MISSION_JSON_MAX - 1) {
      missionLineBuf[missionLineLen++] = c;
    } else {
      missionLineOverflow = true;   // keep reading until '\n', then report the error
    }
  }
}

void setup() {
  Serial.setRxBufferSize(4096);      // default is 256 bytes, a mission can be a few KB. Must be before begin()
  Serial.begin(115200);
  gps_init(GPS_BASE_RX_PIN, GPS_BASE_TX_PIN);
  Serial.println("GPS starting...");

  web_server_init();
  l2r_init();   // after web_server_init(), needs the WiFi access point running
}

void loop() {
  gps_update();
  pollMissionSerial();
  web_server_update();
  l2r_update_rover_telemetry();

  // GPS status over USB once a second (WiFi clients ask for it with GET /api/gps instead)
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.println(gps_status_line());
    Serial.println(l2r_rover_status_line());
  }
}
