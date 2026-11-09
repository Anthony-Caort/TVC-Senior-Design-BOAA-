#include <Arduino.h>
#include <TinyGPSPlus.h>

#define GPS_RX_PIN  16   // ESP32 pin that receives (wire to GPS TX)
#define GPS_TX_PIN  17   // ESP32 pin that transmits (wire to GPS RX)
#define GPS_BAUD    9600

TinyGPSPlus gps;
HardwareSerial GPSserial(2);

void setup() {
  Serial.begin(115200);
  GPSserial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS starting...");
}

void loop() {
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();

    if (gps.charsProcessed() < 10) {
      Serial.println("No data from GPS. Check wiring and baud rate.");
    } else if (gps.location.isValid()) {
      Serial.printf("lat=%.6f  lon=%.6f  sats=%u  hdop=%.1f  speed=%.2f m/s  course=%.1f deg  age=%lu ms\n",
                    gps.location.lat(), gps.location.lng(),
                    gps.satellites.value(), gps.hdop.hdop(),
                    gps.speed.mps(), gps.course.deg(),
                    (unsigned long)gps.location.age());
    } else {
      Serial.printf("No fix yet. sats=%u  (go outside, wait a few minutes)\n",
                    gps.satellites.value());
    }
  }
}