#pragma once

#include <stdint.h>

/*
Base station & rover must agree on these things in the file to talk over WiFi
File is used by both projects (platformio.ini: build_flags = -I../../common)
If anything here changes, rebuild and upload BOTH boards (both ESP32s / comms computers).
*/

// WiFi network. The base station hosts it (is the access point), rover joins it (client)
#define WFC_WIFI_SSID   "BaseStation"
#define WFC_WIFI_PASS   "base12345"      // must be at least 8 characters
#define WFC_BASE_IP     "192.168.4.1"
#define WFC_UDP_PORT    2200            // both boards send/listen on this port (scout22, awesome!!)



/*
Messages sent between the rover and base start with the magic number, otherwise it's not processed
- Msg Start Bytes: Password more or less, allows the rover/base to know a message is from each other and not some random device on the network
                    check out sync word/number
*/
#define WFC_MSG_START_BYTES 0x2077


typedef enum: uint8_t { //this forces the enums to be stored as uint8 (1byte) instead of the default 4bytes 
    ROVER_2_BASE_TELEMETRY = 1,
} wfc_msg_type_t;

//used by both rover & base internally
typedef enum: uint8_t {
    GPS_NODATA = 0,   // no communication from GPS module (bad wiring/baud)
    GPS_NOFIX  = 1,   // GPS talking, no fix on position (yet)
    GPS_FIX    = 2,   // lat & lon are valid
} gps_state_t;



/*
Rover to base status update, sent every L2B_SEND_PERIOD_MS
For now only GPS
Packed & sent as raw bytes since the ESPs will see it / process it the same
*/
typedef struct __attribute__((packed)) { //packed means to not used any padding bytes in between fields
    
    
    // "header" of the message, like the ID of a CAN scheme, these 3 bytes are the same in any message
    uint16_t       msg_start_bytes;
    wfc_msg_type_t msg_type;

    uint32_t        msg_counter;     // counts up by 1 every message, a discontinuity means packets were lost
    uint32_t        rover_uptime_ms; // rover's millis() when sent (uptime, also shows if the rover rebooted)

    // GPS
    gps_state_t     gps_state;
    uint8_t         sats;
    float           hdop;           // lower = better, ~1 is good
    double          lat;            // only valid when gps_state == GPS_FIX
    double          lon;


} rover_telemetry_t;










































// Fails the build if the struct size changes by accident (2+1 + 4+4 + 1+1+4+8+8 = 33 bytes)
static_assert(sizeof(rover_telemetry_t) == 33, "rover_telemetry_t size changed, update this check");
