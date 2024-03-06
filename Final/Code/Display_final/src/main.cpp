#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <AccelStepper.h>

// Define the pins for the stepper motor
#define motorPin1 1 // Motor pin 1
#define motorPin2 2 // Motor pin 2
#define motorPin3 3 // Motor pin 3
#define motorPin4 4 // Motor pin 4

// Initialize the stepper motor using the AccelStepper library
AccelStepper stepper(AccelStepper::FULL4WIRE, motorPin1, motorPin2, motorPin3, motorPin4);

// BLE UUIDs remain the same as you defined
static BLEUUID serviceUUID("fdbbd9d6-8722-46dd-8799-42c17d554582");
static BLEUUID charUUID("4c7a96a1-7634-4d91-b084-7778cbccabda");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// This callback will be invoked when the BLE device we're connected to sends a notification.
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.println("Notify callback for characteristic ");
    // Assuming the data is a simple text command. Adjust as necessary for your protocol.
    String command = String((char*)pData).substring(0, length);
    
    if (command == "reset") {
        // Reset stepper to zero position if "reset" command is received.
        stepper.setCurrentPosition(0);
        stepper.moveTo(0);
    }
    // Log received data for debugging.
    Serial.print("Received data: ");
    Serial.println(command);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);  // Connect to the BLE Server.
    Serial.println(" - Connected to server");

    // Increase the MTU size (optional but can be useful for larger data transfers).
    pClient->setMTU(517);

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Register for notifications on the characteristic.
    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;  // Set to false to avoid continuous scanning once connected.
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Setup the stepper motor
  stepper.setMaxSpeed(1000);  // Maximum speed in steps per second
  stepper.setAcceleration(500);  // Acceleration in steps per second squared

  // Start scanning for BLE servers with a specified service UUID.
  BLEScan* p

