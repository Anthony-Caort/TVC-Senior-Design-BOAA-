#include "gps.h"

//NMEA sentence parser, also stores the last decoded values (location, sats, hdop, ...)
static TinyGPSPlus gps_parser;

//acts like HAL's UART object, we're using UART2
//it also has an RX buffer, unlike HAL
static HardwareSerial gps_uart(2);

void gps_init() {
    // large enough ring buffer, allows web requests can keep the superloop busy for a bit
    // 9600baud / 1024 bytes ~ 960 bytes/s --> 1 second of GPS data 
    // so we can store up to 1 second of GPS data while the superloop is busy
    gps_uart.setRxBufferSize(1024);
    
    //sets up the UART2 periph w/ baud rate, UART format (8N1), tx & rx pins
    //intalls the driver, allocates the buffer, enables the RX interrupt
    //bytes recevied on the RX get put into the buffer via the interrupt
    gps_uart.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}


void gps_update() { //empties the read buffer for the gps, encodes it to NMEA, values stored in the parser
    //.available returns how many bytes left in the ring buffer
    //.read takes the oldest byte, returns it, then deletes it from buffer, FIFO
    /*
    .encode gives the parser on char at a time, the parser extracts(decodes, however named this needs to be sued) the NMEA sentence
    once it hits the checksum and confirms it to be valid, when passed, the values are dropped into their respective fields (.location, .satellites, etc)
    if the checksum isn't passed, sentence gets thrown away & failedCheckSum increases
    */
    while (gps_uart.available()) { 
        gps_parser.encode(gps_uart.read()); 
    }
}



String gps_status_line() {
    char buf[96];

    if (gps_parser.charsProcessed() < 10) { //.charsProcessed returns total chars passed into .encode
        return "GPS,NODATA"; //less than 10 chars gives us nothing, likely faulty module
    } else if (gps_parser.location.isValid()) { // returns true if at least one valid NMEA sentence has cpntrained a position, meaninf if we've gotten a FIX

        //build the sentence with relevan values, 7 decimals precision
        //note that lat & lng are stored as billionths of a degree, we're capping it at 7  
        snprintf(buf, sizeof(buf), "GPS,FIX,%.7f,%.7f,%u,%.1f",
                    gps_parser.location.lat(), gps_parser.location.lng(),
                    gps_parser.satellites.value(), gps_parser.hdop.hdop());

    } else {

        //otherwise we have no fix, but at least GPS is talking
        snprintf(buf, sizeof(buf), "GPS,NOFIX,%u", gps_parser.satellites.value());

    }

    return String(buf); //return constructed string
}
