#include "link_2_base.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#include "gps.h"

/*
UDP = send a packet to an IP + port, no connection, no reply, no resend if it's lost.
Good fit for periodic telemetry: a lost update doesn't matter, the next one replaces it a second later.
(TCP, what HTTP uses, would hold up newer data to resend old data we no longer care about)
*/
static WiFiUDP udp_link;

static uint32_t current_msg_count = 0;     // goes into rover_telemetry_t.msg_counter, +1 per message
static uint32_t latest_tx_ms      = 0;     // millis() when the last msg was sent
static bool     wifi_is_connected = false; // last known state, to print only when it changes


void l2b_init() {
    WiFi.mode(WIFI_STA);              // rover = client, joins someone else's network
    WiFi.setAutoReconnect(true);      // if connection to base is lost, keep retrying on its own
    WiFi.begin(WFC_WIFI_SSID, WFC_WIFI_PASS);   // starts connecting in the background, returns right away

    udp_link.begin(WFC_UDP_PORT);    // open our UDP port (for sending now, receiving missions later)

    Serial.println("Joining WiFi '" WFC_WIFI_SSID "'...");
}

// Build one telemetry message from the latest GPS values and send it to the base station
static void l2b_send_telemetry() {
    gps_reading_t curr_gps_reading;
    gps_get_reading(&curr_gps_reading);

    rover_telemetry_t outgoing_msg = {};       // = {} zeroes every field first
    outgoing_msg.msg_start_bytes = WFC_MSG_START_BYTES;
    outgoing_msg.msg_type        = ROVER_2_BASE_TELEMETRY;
    outgoing_msg.msg_counter     = current_msg_count++;
    outgoing_msg.rover_uptime_ms = millis();
    outgoing_msg.gps_state       = curr_gps_reading.state;
    outgoing_msg.sats            = curr_gps_reading.sats;
    outgoing_msg.hdop            = curr_gps_reading.hdop;
    outgoing_msg.lat             = curr_gps_reading.lat;
    outgoing_msg.lon             = curr_gps_reading.lon;

    // the struct's raw bytes are the packet, the base station copies them straight back into a rover_telemetry_t variable as well
    udp_link.beginPacket(WFC_BASE_IP, WFC_UDP_PORT);
    udp_link.write((const uint8_t*)&outgoing_msg, sizeof(outgoing_msg));
    udp_link.endPacket();
}

void l2b_update() {
    // WL_CONNECTED = joined the network AND got an IP address from the base station
    bool curr_connection_status = (WiFi.status() == WL_CONNECTED);

    if (curr_connection_status != wifi_is_connected) {
        wifi_is_connected = curr_connection_status;
        if (wifi_is_connected) {
            Serial.print("WiFi connected, rover IP: ");
            Serial.println(WiFi.localIP());   // 192.168.4.2 if it's the first device to join
        } else {
            Serial.println("WiFi lost, reconnecting...");
        }
    }

    if (!wifi_is_connected) {
        return;   // nowhere to send to
    }

    if (millis() - latest_tx_ms >= L2B_SEND_PERIOD_MS) {
        latest_tx_ms = millis();
        l2b_send_telemetry();
    }
}
