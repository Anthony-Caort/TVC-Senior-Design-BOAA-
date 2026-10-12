#include "BLEDevice.h"
#define LED_PIN 8
unsigned long lastHeartbeat = 0;

//BLE Server name (the other ESP32 name running the server sketch)
#define bleServerName "BME280_ESP32"

/* UUID's of the service, characteristic that we want to read*/
// BLE Service
static BLEUUID droneServiceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");

// Humidity Characteristic
static BLEUUID droneTimerCharacteristicUUID("ca73b3ba-39f6-4ab3-91ae-186dc9577d99");

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

//Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;

//Characteristicd that we want to read
static BLERemoteCharacteristic* droneTimerCharacteristic;

//Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

//Variables to store temperature and humidity
String timerChar = "";

//Flags to check whether new temperature and humidity readings are available
bool newTimer = false;

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
   BLEClient* pClient = BLEDevice::createClient();
 
  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");
 
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(droneServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(droneServiceUUID.toString().c_str());
    return (false);
  }
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  droneTimerCharacteristic = pRemoteService->getCharacteristic(droneTimerCharacteristicUUID);
  //humidityCharacteristic = pRemoteService->getCharacteristic(humidityCharacteristicUUID);

  /*if (temperatureCharacteristic == nullptr || humidityCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }*/

  if (droneTimerCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");
 
  //Assign callback functions for the Characteristics
  droneTimerCharacteristic->registerForNotify(droneTimerNotifyCallback);
  return true;
}

//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
public:
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
  }
};
 
//When the BLE Server sends a new temperature reading with the notify property
static void droneTimerNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {
  //store temperature value
  timerChar = "";
  for (size_t i = 0; i < length; i++) {
    timerChar += (char)pData[i];
  }
  newTimer = true;
}

//When the BLE Server sends a new humidity reading with the notify property
/*static void humidityNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  //store humidity value
  humidityChar = (char*)pData;
  newHumidity = true;
  Serial.print(newHumidity);
}*/

void setup() {
  //Start serial communication
  Serial.begin(115200);
  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 5000)) {
    delay(10);
  }
  Serial.println("Starting Arduino BLE Client application...");

  pinMode(LED_PIN, OUTPUT);
  //Init BLE device
  BLEDevice::init("");
 
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop() {
  if (millis() - lastHeartbeat > 3000) {
    lastHeartbeat = millis();
    if (connected) {
      Serial.println("[Status] Connected to Server. Awaiting BLE notifications...");
    } else if (doConnect) {
      Serial.println("[Status] Match found! Instantiating handshake...");
    } else {
      Serial.println("[Status] Scanning for BME280_ESP32...");
    }
  }
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      //Activate the Notify property of each Characteristic
      droneTimerCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      //humidityCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
    } else {
      Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }
  //if new temperature readings are available, print in the OLED
  if (newTimer){
    newTimer = false;
    Serial.println(timerChar);
  }

  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  delay(250); // Delay a second between loops.

}
