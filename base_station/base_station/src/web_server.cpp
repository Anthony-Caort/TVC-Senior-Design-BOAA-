#include "web_server.h"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include "gps.h"
#include "mission.h"
#include "link_2_rover.h"

static WebServer http_server(80);    // port 80 = normal http://
static bool fs_mounted = false;      // LittleFS holds the UI files (index.html, Leaflet, tiles), needed before reads

/* Alan's notes (for file system stuff)

An ESP32 has a flash memory chip of 4 MB of raw data. LittleFS sets up a file system within the flash
The filesystem has a table of contents of what files are "called", which areas of the memory hold the file and how large the file is
The rest of the filesystem is the files themselves

Mounting a filesystem (at least for LittleFS) means that it 
- Checks the storage partition (specified by no_ota.csv)
- Reads the table of contents, checks that its a valid LittleFS 
- Sets up a state in RAM so that "LittleFS.open("/index.html.gz") knows where to look." (state in RAM refers to important bits)

Now the flash of an ESP32 is partitioned into sections, two of them are 
- app = the compiled C++ program             | written to the ESP via "Upload" in VScode
- storage = UI files (HTML, Leaflet, tiles)  | written to the ESP via "Upload Filesystem Image"  in VScode('s PlatformIO's command bar)

make_fs_data.py 
- It prepares the files to be stored on the ESP32
- From the workspace
    * Takes the files needed for the webUI from the /UI folder and puts them in the /data folder (workspace)
    * index.html, leaflet.js & .css get compressed (GZIPPED so that's .gz) (gzip is a compression format)
    * .jpg and .png images are already compressed so those are just copied 
- It makes a config.js file
- Prints how full the storage ends up


Upload Filesystem Image
- Runs make_fs_data.py, now that all the webUI stuff is in /data UFI takes over
- All of this data gets put into a .bin "image" file, in LittleFS format, & writes that image to the storage partition of the ESP32
- PlatformIO runs the script automatically before every build 
    * platformio.ini has extra_scripts = pre:tools/make_fs_data.py. 
    * data/ is always rebuilt from the latest UI/ folder 
    * That’s also why data/ is in .gitignore: it’s generated, and UI/ is the real copy.

Partitioning & no_ota.csv
- The partitions of the flash of the ESP32 are described by a partition table
- no_ota.csv is a premade partition table
    * comes with the ESP32 Arduino core, platformio.ini picks it with board_build.partitions = no_ota.csv
- By choosing no OTA (over the air) firmware updates we have extra storage to use 
*/

/* Alan's notes (for web serve stuff)

How wifi works
- A server (the ESP32) has files, it waits for someone to ask for the files, never asks first
- A client (the browser in your laptop/phone) wants files, it asks for them, 

IP & Port
- IP is the address/ID of a device on the network 
    * ESP32 gives itself (192.168.4.1) & .2 & .3 etc to subsequent devices that join  
- Port is a "door" at an IP, one device may run different services on different ports
    * 80 is the default for web pages, browsers default to it unless told otherwise 

HTTP = Hypertext Transfer Protocol 
- Plain text lagnuage for wifi communication over wifi radio (not specific to wifi though)

Browser asking for GPS data (HTTP)
GET /api/gps HTTP/1.1
Host: 192.168.4.1

ESP32 responds with 
HTTP/1.1 200 OK
Content-Type: text/plain
Cache-Control: no-store

GPS,FIX,33.9733448,-117.3281939,9,0.9

- Every exchange between the browser and ESP32 (client & server) is a 1 request & 1 response 
- handleClient() reads a request, send() write a response

Server response 
- Status code = (200 - OK) (400 - bad request) (404 - unknown request) (500 - server problem)
- Headers = labels about the response (e.g. Content-Type & Cache-Control)
- Body = content of the response/request (e.g. GPS line, HTML file, tile image, etc)

Client Request
- Method = first word of a request
    * GET = requesting something/data, no body (loading a page/tile getting the GPS line)
    * POST = giving data, in the body of the request (sending a mission, the JSON would be the body)

E.G. (sending a mission to the server from the UI)
POST /api/mission HTTP/1.1
Host: 192.168.4.1
Content-Length: 72

{"waypoints":[{"id":1,"lat":33.9740,"lon":-117.3290}],"boundary":[]}

- In the code "http_server.on("/api/mission", HTTP_POST, handle_api_mission);" maps the handle_api_mission function to a request with POST method & /api/mission path

URL = Uniform Resource Locator
- The full address of something on the network

E.G.
http://192.168.4.1:80/tiles/16/11300/26500.jpg
└─┬─┘  └────┬────┘ └┬┘└────────────┬─────────┘
scheme    host    port           path

- Scheme: the language to speak (http)
- Host: Which device the something is on (192.168.4.1 AKA the server/ESP32)
- Port: Which door/service at the device (80)
- Path: What thing do we want from the Host, appears on the request line (e.g. GET /tiles/16/...)

"URL Folder"
- Paths used by URLs mimic folders in a computer, here it's just text and basically nothing more than variables we can map
- http_server.serveStatic("/tiles", LittleFS, "/tiles", "max-age=604800");
    * Here, we map the “/tiles” path to the real “/tiles” folder from LittleFS
    * So here the URL Folder and File Folder actually match 
- http_server.on("/", HTTP_GET, handle_index);
    * We route the “/” path to the handle_index function which sends the “index.html.gz” file
    * There is no file named “/” 
- http_server.on("/api/gps", HTTP_GET, handle_api_gps);
    * There is no “api” file/folder nor is there a “gps” file/folder
    * This path (& method) simply trigger the “handle_api_gps” function

Opening the UI on a browser
0. Type 192.168.4.1 on your browser
1. The browser sends GET / to the ESP32 on port 80. The ESP32 sends back the HTML.
2. The HTML says to load lib/leaflet.js, so the browser sends GET /lib/leaflet.js.
3. The map needs tiles, so the browser sends about 20 requests like GET /tiles/16/.../....jpg.
4. Once a second, the page’s JavaScript sends GET /api/gps.
5. When you press Send, the JavaScript sends POST /api/mission with the JSON in the body.

*/

/* Alan's notes, The UI itself

index.html
- The actual "UI Code" contains three languages and tells the browser how to build the UI
    * Structure of the webpage - HTML - Where the map goes, buttons, log window
    * Looks - CSS (<style> block) - Colors/sizes/layout
    * Behavior - JavaScript (<script> block) - adds waypoints, talks to ESP over USB/wifi, builds the JSON, etc
- NOTE: The browser is what actually runs the code/UI, the ESP just stores all these files

Leaflet
- JavaScript library that handles interactive maps & everything that makes up the UI's map
- Comes as .js and .css, one for code another for styling alongside some icons
- Copies of those are in /lib on the ESP32, UI/lib on the laptop, and are typically pulled from the internet

Tiles
- 256x256 pixel images that make up the map
- zn denotes the zoom level, each zoom level has 4x as many images as before
- all stored in the tiles folder
*/


// Triggers on the route "/" -> index.html (stored gzipped as /index.html.gz, browser unzips it)
static void handle_index() {
    //if fs is mounted, open the gzipped index.html in "r" read mode, put that in index_file
    //otherwise, give it an invalid File
    File index_file = fs_mounted ? LittleFS.open("/index.html.gz", "r") : File();

    //if given an invalid File, it acts as a false, tripping this
    if (!index_file) {
        http_server.send(500, "text/plain",
                         "UI files missing on the ESP32. In PlatformIO run 'Upload Filesystem Image', then reset the board.");
        return;
    }

    //queue header, stream file in chunks, close/release file
    http_server.sendHeader("Cache-Control", "no-cache");   // always re-check, so a new upload shows up right away
    http_server.streamFile(index_file, "text/html");
    index_file.close();
}

// Trigger on route GET /api/gps -> current GPS status line (same format as USB)
static void handle_api_gps() {
    http_server.sendHeader("Cache-Control", "no-store"); //doesnt store a copy on browser, no stale values on refresh
    http_server.send(200, "text/plain", gps_status_line()); //sends a string gps line
}

// Trigger on route POST /api/mission -> body is the mission JSON line, reply is the ACK/ERR text
static void handle_api_mission() {
    if (!http_server.hasArg("plain")) {  //checks if reqest has body
        http_server.send(400, "text/plain", "ERR empty body\n"); //if not returns error
        return;
    }

    const String& body = http_server.arg("plain"); //.arg returns the body as string, we name it
    if (body.length() >= MISSION_JSON_MAX) {
        http_server.send(413, "text/plain", "ERR mission too long\n"); //error for too long a mission
        return;
    }

    //turn the body in a const char*, instead of String & store the ack for the mission
    String reply = mission_process_json(body.c_str()); 
    Serial.print("WIFI ");      // also show WiFi mission acks on USB, handy for debugging
    Serial.print(reply);
    http_server.send(200, "text/plain", reply); //send ack to UI
}

static void handle_api_rover () { //written to Push Rope - Static Dress
    http_server.sendHeader("Cache-Control", "no-store"); //doesnt store a copy on browser, no stale values on refresh
    http_server.send(200, "text/plain", l2r_rover_status_line()); //sends the string of the rover's gps fix
}

void web_server_init() {
    fs_mounted = LittleFS.begin(false);   // if returned false = mount failed = don't format 
    if (!fs_mounted) {
        Serial.println("LittleFS mount failed. Run 'Upload Filesystem Image' in PlatformIO.");
    }

    //set wifi mode for radio, access point only
    WiFi.mode(WIFI_AP);
    
    //configure access point, starts broadcasting the network
    bool ap_ok = WiFi.softAP(WEB_AP_SSID, WEB_AP_PASS, 1, 0, 4);   // ssid, pass, channel, hidden, max clients
    
    //check wifi setup ok
    Serial.println(ap_ok ? "AP started" : "AP failed");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());                            // 192.168.4.1 by default

    //registers routes for the server, specifies the function to call per URL (gives a pointer)
    http_server.on("/", HTTP_GET, handle_index);                    
    http_server.on("/index.html", HTTP_GET, handle_index);  
    http_server.on("/api/gps", HTTP_GET, handle_api_gps);
    http_server.on("/api/rover", HTTP_GET, handle_api_rover);
    http_server.on("/api/mission", HTTP_POST, handle_api_mission); //url, method, function
    

    // server routes for static files
    // must be set up AFTER LittleFS.begin(), serveStatic checks the paths when it is called, it needs the filesystem
    // Leaflet is stored as .gz, serveStatic finds leaflet.js.gz automatically and sends it gzipped.
    if (fs_mounted) {
        http_server.serveStatic("/config.js", LittleFS, "/config.js", "no-cache"); //url_prefix, filesystem, folder, cache-header
        http_server.serveStatic("/lib", LittleFS, "/lib", "max-age=604800");      // cache 1 week, never changes
        http_server.serveStatic("/tiles", LittleFS, "/tiles", "max-age=604800");
    }

    http_server.onNotFound([]() { http_server.send(404, "text/plain", "Not found"); });
    http_server.begin(); //opens the TCP socket on port 80, browsers(clients) can connect, requests are only handled by handleClient() though
}


void web_server_update() {
    /*
    handleClient() - checks if a browser is connected (if not it returns immidietly
    If there is a browser connected, it reads a request (method, URL, headers & body)
    Checks the list of the routings we made and calls the handler for the first match
    Handler is the what sends the response and the connection is finished
    While its running it blocks the rest of the code, could be up to 5 seconds in WCS (not World Cutting Slash btw)
    */
    http_server.handleClient();
}
