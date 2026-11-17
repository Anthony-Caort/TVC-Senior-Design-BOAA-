#include "link_2_rover.h"
#include <WiFi.h>
#include <WiFiUdp.h>

static WiFiUDP udp_link;   // listens on WFC_UDP_PORT for packets from the rover

static rover_telemetry_t latest_rx_telemetry;   // newest valid message from the rover
static uint32_t latest_rx_ms = 0;              // millis() when newest msg arrived
static bool     have_1st_telemetry = false;    // false until the first valid message, meaning we have A valid msg


void l2r_init() {
    udp_link.begin(WFC_UDP_PORT);
}

void l2r_update_rover_telemetry() {
    int packet_size;
    while ((packet_size = udp_link.parsePacket()) > 0) {  // parsePacket() takes the next waiting packet (0 = none), loop until all are read

        // wrong size = not a rover_telemetry_t, skip it TODO: Add code to accept other structs (if made)
        if (packet_size == sizeof(rover_telemetry_t)) {
            rover_telemetry_t received_msg;
            udp_link.read((uint8_t*)&received_msg, sizeof(received_msg));   // raw bytes straight back into the struct

            if (received_msg.msg_start_bytes == WFC_MSG_START_BYTES && received_msg.msg_type == ROVER_2_BASE_TELEMETRY) {
                latest_rx_telemetry = received_msg; //get msg contents
                latest_rx_ms        = millis(); //mark current time
                have_1st_telemetry  = true; //set every time we get telem
            }
        }

        udp_link.flush();   // drop whatever is left of this packet before taking the next one
    }
}

bool l2r_get_rover_telemetry(rover_telemetry_t* copy_into) {
    if (!have_1st_telemetry || millis() - latest_rx_ms > L2R_TIMEOUT_MS) {
        return false; //if we dont have telemetry or the current telemetry is stale
    }
    *copy_into = latest_rx_telemetry; //otherwise, get the telemetry
    return true;
}

String l2r_rover_status_line() {
    rover_telemetry_t curr_rover_telem;
    if (!l2r_get_rover_telemetry(&curr_rover_telem)) { //if l2r_get_rover_telemetry return false we dont currently have a link
        return "ROVER,NOLINK";
    }

    char buf[96];
    if (curr_rover_telem.gps_state == GPS_FIX) {
        snprintf(buf, sizeof(buf), "ROVER,FIX,%.7f,%.7f,%u,%.1f",
                 curr_rover_telem.lat, curr_rover_telem.lon, curr_rover_telem.sats, curr_rover_telem.hdop);
    } else if (curr_rover_telem.gps_state == GPS_NOFIX) {
        snprintf(buf, sizeof(buf), "ROVER,NOFIX,%u", curr_rover_telem.sats);
    } else {
        snprintf(buf, sizeof(buf), "ROVER,NODATA");
    }
    return String(buf);
}
