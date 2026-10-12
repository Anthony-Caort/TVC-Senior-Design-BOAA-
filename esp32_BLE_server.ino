#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
//#include <Wire.h>
//#include <Adafruit_Sensor.h>
//#include <Adafruit_BME280.h>


//BLE server name
#define bleServerName "BME280_ESP32"

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 500;

bool deviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"


// Humidity Characteristic and Descriptor
BLECharacteristic droneTimeCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor droneTimeDescriptor(BLEUUID((uint16_t)0x2903));

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};


void setup() {
  //Serial.setTxTimeoutMs(0); 
  // Start serial communication 
  Serial.begin(115200);


  // Create the BLE Device
  BLEDevice::init(bleServerName);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *droneService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics and Create a BLE Descriptor

  // Humidity
  droneService->addCharacteristic(&droneTimeCharacteristics);
  droneTimeDescriptor.setValue("BME humidity");
  droneTimeCharacteristics.addDescriptor(new BLE2902());
  
  // Start the service
  droneService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    if ((millis() - lastTime) > timerDelay) {
      //Set humidity Characteristic value and notify connected client
      droneTimeCharacteristics.setValue(String(lastTime));
      droneTimeCharacteristics.notify();   
      Serial.print(" - Humidity: ");
      Serial.print(lastTime);
      Serial.println(" %");
      
      lastTime = millis();
    }
  }
}