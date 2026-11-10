#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include "gps.h"
#include "mission.h"

// WiFi access point the phone/laptop joins to open the UI at http://192.168.4.1
const char* AP_SSID = "BaseStation";
const char* AP_PASS = "base12345";   // must be at least 8 characters

WebServer server(80);
bool fsMounted = false;   // LittleFS holds the UI files (index.html, Leaflet, tiles)

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

// ---------- WiFi web server ----------
// "/" -> index.html (stored gzipped as /index.html.gz, browser unzips it)
void handleIndex() {
  File f = fsMounted ? LittleFS.open("/index.html.gz", "r") : File();
  if (!f) {
    server.send(500, "text/plain",
                "UI files missing on the ESP32. In PlatformIO run 'Upload Filesystem Image', then reset the board.");
    return;
  }
  server.sendHeader("Cache-Control", "no-cache");   // always re-check, so a new upload shows up right away
  server.streamFile(f, "text/html");
  f.close();
}

// GET /api/gps -> current GPS status line (same format as USB)
void handleApiGps() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/plain", gps_status_line());
}

// POST /api/mission -> body is the mission JSON line, reply is the ACK/ERR text
void handleApiMission() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "ERR empty body\n");
    return;
  }
  const String& body = server.arg("plain");
  if (body.length() >= MISSION_JSON_MAX) {
    server.send(413, "text/plain", "ERR mission too long\n");
    return;
  }
  String reply = mission_process_json(body.c_str());
  Serial.print("WIFI ");      // also show WiFi missions on USB, handy for debugging
  Serial.print(reply);
  server.send(200, "text/plain", reply);
}

void setupWebServer() {
  WiFi.mode(WIFI_AP);                                 // act as an access point only
  bool ok = WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 4);   // ssid, pass, channel, hidden, max clients
  Serial.println(ok ? "AP started" : "AP failed");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());                    // 192.168.4.1 by default

  server.on("/", HTTP_GET, handleIndex);
  server.on("/index.html", HTTP_GET, handleIndex);
  server.on("/api/gps", HTTP_GET, handleApiGps);
  server.on("/api/mission", HTTP_POST, handleApiMission);

  // Static files. Must be set up AFTER LittleFS.begin(), serveStatic checks the paths when it is called.
  // Leaflet is stored as .gz, serveStatic finds leaflet.js.gz automatically and sends it gzipped.
  if (fsMounted) {
    server.serveStatic("/config.js", LittleFS, "/config.js", "no-cache");
    server.serveStatic("/lib", LittleFS, "/lib", "max-age=604800");      // cache 1 week, never changes
    server.serveStatic("/tiles", LittleFS, "/tiles", "max-age=604800");
  }

  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

void setup() {
  Serial.setRxBufferSize(4096);      // default is 256 bytes, a mission can be a few KB. Must be before begin()
  Serial.begin(115200);
  gps_init();
  Serial.println("GPS starting...");

  fsMounted = LittleFS.begin(false);   // false = don't format if mount fails, the UI files would be lost
  if (!fsMounted) {
    Serial.println("LittleFS mount failed. Run 'Upload Filesystem Image' in PlatformIO.");
  }

  setupWebServer();
}

void loop() {
  gps_update();
  pollMissionSerial();
  server.handleClient();

  // GPS status over USB once a second (WiFi clients ask for it with GET /api/gps instead)
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.println(gps_status_line());
  }
}
