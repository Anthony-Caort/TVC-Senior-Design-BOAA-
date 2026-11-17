#include <Arduino.h>

#include "gps.h"
#include "link_2_base.h"

void setup() {
  Serial.begin(115200);
  gps_init(GPS_ROVER_RX_PIN, GPS_ROVER_TX_PIN);
  Serial.println("Rover GPS starting...");

  l2b_init();
}

void loop() {
  gps_update();
  l2b_update();

  // GPS status over USB once a second, for debugging on the bench
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.println(gps_status_line());
  }
}
