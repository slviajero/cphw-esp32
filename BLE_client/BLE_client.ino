/**
 * Based on BLE client example.
 */

#include "BLEDevice.h"

#include <sstream>

// REPLACE THIS WITH YOUR PASSCODE
// #define PASSCODE "101112131415161718191A1B1C1D1E1F"
#define PASSCODE "23BB798C0866C785E444E26419CDC5DA"


// the advertised service
static BLEUUID advertServiceUUID("52756265-6e43-6167-6e69-654350485000");
// The remote service we wish to connect to.
static BLEUUID serviceUUID("52756265-6e43-6167-6e69-654350485100");
// The characteristic of the remote service we are interested in.
static BLEUUID serialNumberUUID("52756265-6e43-6167-6e69-654350485105");
static BLEUUID passwordUUID("52756265-6e43-6167-6e69-654350485101");

// A few more things from the pc code using this 
/*
110             static CPHW_SERVICE_UUID = '52756265-6e43-6167-6e69-654350485000'
111             static CPHW_OTHER_UUID = '52756265-6e43-6167-6e69-654350485001'
112             static RIDE_MODE_UUID = '52756265-6e43-6167-6e69-65435048500a'
113             static CPHW_BATTERY_UUID = '52756265-6e43-6167-6e69-65435048500b'
114             static CPHW_SERVICE_UUID_100 = '52756265-6e43-6167-6e69-654350485100'
115             static AUTH_REQUEST_UUID = '52756265-6e43-6167-6e69-654350485101'
116             static AUTH_STATUS_UUID = '52756265-6e43-6167-6e69-654350485104'
117             static BOARD_SERIAL_UUID = '52756265-6e43-6167-6e69-654350485105'
118 
119             static TURBO = "7f55007d00000000804040ffd50000000001"
120             static STANDARD = "40401e00000000007f7f7f7f7f0000000001"
121             static ECO = "20200a00000000007f7f7f7f7f0000000001"
 */
static BLEUUID rideModeUUID("52756265-6e43-6167-6e69-65435048500a");
static BLEUUID cphwBatteryUUID("52756265-6e43-6167-6e69-65435048500b");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pSerialNumberCharacteristic;
static BLERemoteCharacteristic* pPasswordCharacteristic;
static BLEAdvertisedDevice* myDevice;

// from ChatGPT
std::vector<uint8_t> hexStringToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    
    // Ensure the input string has an even number of characters
    if(hex.length() % 2 != 0) {
        Serial.println("Invalid hex string: length must be even.");
        return bytes; // Empty vector
    }

    for (size_t i = 0; i < hex.length(); i += 2) {
        std::stringstream ss;
        ss << std::hex << hex.substr(i, 2);
        uint16_t byte;
        if (!(ss >> byte)) {
            Serial.println("Error converting hex string to byte.");
            return bytes; // Empty vector
        }
        bytes.push_back(static_cast<uint8_t>(byte));
    }
    
    return bytes;
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.write(pData, length);
    Serial.println();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

/* made this global for later use */
BLEClient* pClient;

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    /* make the client variable global */ 
    //BLEClient*  pClient  = BLEDevice::createClient();
    
    pClient  = BLEDevice::createClient(); 
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // // Obtain a reference to the characteristic in the service of the remote BLE server.
    pSerialNumberCharacteristic = pRemoteService->getCharacteristic(serialNumberUUID);

    if (pSerialNumberCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(serialNumberUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    
    // Read the value of the characteristic.
    if(pSerialNumberCharacteristic->canRead()) {
      std::string serialNumber = pSerialNumberCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(serialNumber.c_str());
    }

    pPasswordCharacteristic = pRemoteService->getCharacteristic(passwordUUID);
    if (pPasswordCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(passwordUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }

    Serial.println("writing passcode");
    std::vector<uint8_t> pw = hexStringToBytes(PASSCODE);
    pPasswordCharacteristic->writeValue(pw.data(), 16, true);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(advertServiceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  // pBLEScan->setInterval(1349);
  // pBLEScan->setWindow(449);
  // pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {

    // pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(5000); // Delay a second between loops.
} // End of loop
