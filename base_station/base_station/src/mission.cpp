#include "mission.h"
#include <ArduinoJson.h>

/*
These two arrays store the current mission
static so only this file can see/touch them, other files must use the functions defined below
each element in the arrays is two doubles that make up the geo-point struct, so 16 bytes per point
each array stores 50 points so they weigh in at 800 bytes each
*/
static geo_point_t current_mission_waypoints[MISSION_MAX_WAYPOINTS];
static geo_point_t current_mission_boundary[MISSION_MAX_BOUNDARY_PTS];

/*
how much of the arrays have been used, each array CAN HOLD 50 points
with these two length variables we track the first k elements we're actually using from the above arrays
= 0 means no mission yet
*/
static int current_mission_waypoints_len = 0;
static int current_mission_boundary_len = 0;

/*
EXAMPLE JSON TEXT FROM UI
{"waypoints":[{"id":1,"lat":33.9740,"lon":-117.3290}],"boundary":[]}

Once deserialized we get a tree in memory

{} is an "object", houses key-value pairs like "id":1, "id" is the key
    similar to python dict, accessed via the key AKA name AKA string obj["id"] (returns value 1)

[] is an "array", ordered list, accessed via index arr[0]
*/

String mission_process_json(const char* mission_json) {
    char buf[96]; //buffer for snprintf

    /*
    holds an iterable, parsed version of a JSON file
    more specifically, it consumes heap to make nodes (for the tree), one per object, array, key & value
    these nodes are interconnected via a tree (think linked lists but with branches instead of completely linear)
    freed automatically when the function returns
    */
    JsonDocument mission_doc;

    /*
    deserializeJson parses the JSON and translates it into a JsonDocument object AKA mission_doc AKA a tree
    it returns a DeserializationError object, specifies how the parsing failed, if it failed
    parse_error holds an error code (Ok if parsing worked), inside an if() it acts as true when parsing failed
    */
    DeserializationError parse_error = deserializeJson(mission_doc, mission_json);
    if (parse_error) {
        return String("ERR bad JSON: ") + parse_error.c_str() + "\n"; //parse_error.c_str specifies the failure
    }

    /*
    received_mission_waypoints & received_mission_boundary are like pointers,
    they point to a certain part of mission_doc w/o copying it
    (current_mission_waypoints / current_mission_boundary at the top of this file are our working copy)

    mission_doc["waypoints"] is an array of objects (waypoint objects basically)
    each object has 3 key-value pairs (id, lat, lon)
    though id is kinda ignored since the array indices already specify it

    mission_doc["boundary"] is more or less the same but the objects only have lat & lon
    the objects are the vertices/corners/points upon which the boundary is anchored/drawn

    mission_doc["waypoints"]
    - returns a handle to the "value" stored at the "waypoints" key, type is unspecified and un-needed

    JsonArray received_mission_waypoints = mission_doc["waypoints"]
    - this assignment converts the handle returned from mission_doc["waypoints"]
    - the JsonArray has a pointer to the array node, if the "value" of the key is an array then we're good
    - if it's not an array (or the key is missing) it becomes a null JsonArray (valid but empty handle, 
    - no crash like when dereferencing a null pointer)
    */
    JsonArray received_mission_waypoints = mission_doc["waypoints"];
    JsonArray received_mission_boundary  = mission_doc["boundary"];


    /*
    simple check that received_mission_waypoints is in fact pointing to an array and not a null handle
    the UI will not send an empty waypoint array, but the code doesnt have anything stopping it from accepting it

    since the boundary of the mission is optional we dont check it, a null array has size 0 and loops 0 times
    */
    if (received_mission_waypoints.isNull()) {
        return "ERR no waypoints array\n";
    }

    //check for mission waypoint or boundary vertex overflow
    if (received_mission_waypoints.size() > MISSION_MAX_WAYPOINTS || received_mission_boundary.size() > MISSION_MAX_BOUNDARY_PTS) {
        snprintf(buf, sizeof(buf), "ERR too many points (max %d waypoints, %d boundary)\n",
                 MISSION_MAX_WAYPOINTS, MISSION_MAX_BOUNDARY_PTS);
        return String(buf);
    }

    /*
    So now that we know we have some waypoints & they dont overflow, we can store the mission
    or overwrite an old mission
    */

    /*
    resets the count to "no points", the old values stay in the array until they get overwritten in the for loop below
    it is also used as an iterable index in the loop
    */
    current_mission_waypoints_len = 0;

    /*
    wp is a handle/pointer to the current element of received_mission_waypoints, gets pointed to the next element every loop
    basically similar notation to a python for loop

    wp["lat"] gets the value of the "lat" key-value pair, converted to double on assignment
    */
    for (JsonObject wp : received_mission_waypoints) {
        current_mission_waypoints[current_mission_waypoints_len].lat = wp["lat"];
        current_mission_waypoints[current_mission_waypoints_len].lon = wp["lon"]; //same as above for lon
        current_mission_waypoints_len++;    //move to the next index for current mission arrays, also counts the points
    }

    //same as above, constructs the new boundary array for the new mission
    current_mission_boundary_len = 0;
    for (JsonObject pt : received_mission_boundary) {
        current_mission_boundary[current_mission_boundary_len].lat = pt["lat"];
        current_mission_boundary[current_mission_boundary_len].lon = pt["lon"];
        current_mission_boundary_len++;
    }

    // Makes the 1-line ACK of the overall mission
    snprintf(buf, sizeof(buf), "ACK %d waypoints, %d boundary pts\n", current_mission_waypoints_len, current_mission_boundary_len);

    /*
    On top of the 1-line ACK we also describe each waypoint
    each line is formatted in buf with snprintf (it does %.7f formatting), then appended to ack
    */
    String ack = buf;
    for (int i = 0; i < current_mission_waypoints_len; i++) {
        snprintf(buf, sizeof(buf), "  WP%d lat=%.7f lon=%.7f\n", i + 1, current_mission_waypoints[i].lat, current_mission_waypoints[i].lon);
        ack += buf;
    }

    // TODO: forward mission to rover over radio

    return ack;
}

//getters, other files can read with these functions, but only the big function above can change the counters
int mission_waypoint_count() {return current_mission_waypoints_len;}
int mission_boundary_count() {return current_mission_boundary_len;}

//index is the waypoint # we're getting, out is a pointer to the caller's object/variable so the function can fill its values
bool mission_get_waypoint(int index, geo_point_t* out) {
    if (index < 0 || index >= current_mission_waypoints_len) { //check it's within range
        return false;
    }
    *out = current_mission_waypoints[index]; //copy the point into the caller's variable
    return true;
}

//same as above
bool mission_get_boundary_point(int index, geo_point_t* out) {
    if (index < 0 || index >= current_mission_boundary_len) {
        return false;
    }
    *out = current_mission_boundary[index];
    return true;
}
