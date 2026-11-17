// Settings that differ between the laptop copy of the UI and the copy the ESP32 serves.
// This file is the LAPTOP version (served by `python -m http.server` from this folder).
// The ESP32 version is generated into base_station/base_station/data/config.js by
// base_station/base_station/tools/make_fs_data.py, don't edit that copy by hand.
window.UI_CONFIG = {
  transport: 'usb',     // 'usb'  = talk to the base station over USB (Web Serial, Chrome/Edge)
                        // 'wifi' = page is served by the ESP32, talk to it over HTTP
  maxNativeZoom: 18,    // highest zoom level that has tiles in tiles/ (laptop has z15-18)
  // [[south, west], [north, east]] that every zoom in tiles/ covers, the map can't leave it.
  // Re-exported tiles? Copy the "laptop UI/config.js tileBounds" line make_fs_data.py prints on every PlatformIO build.
  tileBounds: [[33.9695601, -117.33673], [33.9809465, -117.3202525]]
};
