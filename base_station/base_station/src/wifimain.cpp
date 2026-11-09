// #include <Arduino.h>
// #include <WiFi.h>
// #include <WebServer.h>


// const char* AP_SSID = "BaseStation";
// const char* AP_PASS = "base12345";   // must be at least 8 characters

// WebServer server(80);

// void handleRoot() {
//   server.send(200, "text/html", "<h1>Base station UI will be here in the future</h1>");
// }

// void setup() {
//   Serial.begin(115200);
//   delay(500);

//   WiFi.mode(WIFI_AP);                                 // act as an access point only
//   bool ok = WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 4);   // ssid, pass, channel, hidden, max clients

//   Serial.println(ok ? "AP started" : "AP failed");
//   Serial.print("AP IP: ");
//   Serial.println(WiFi.softAPIP());                 // 192.168.4.1 by default

//   server.on("/", handleRoot);
//   server.begin();
// }

// void loop() {
//   Serial.print("Connected clients: ");
//   Serial.println(WiFi.softAPgetStationNum());
//   delay(2000);
//   server.handleClient();
// }