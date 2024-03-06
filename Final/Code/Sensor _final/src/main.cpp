#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;  // Interval for sending data

#define SERVICE_UUID        "fdbbd9d6-8722-46dd-8799-42c17d554582"
#define CHARACTERISTIC_UUID "4c7a96a1-7634-4d91-b084-7778cbccabda"

// Moving average filter settings
const int numReadings = 10;  // Number of readings for the moving average
float readings[numReadings]; // the readings from the analog input
int readIndex = 0;           // the index of the current reading
float total = 0;             // the running total
float average = 0;           // the average

const float intenseMovementThreshold = 1.5;  // Adjust based on your needs

Adafruit_MPU6050 mpu;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) override {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) { delay(10); }
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    Serial.println("MPU6050 Found!");

    // Initialize moving average filter
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
    }

    BLEDevice::init("MPU6050 Sensor");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    if (deviceConnected) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            sensors_event_t a, g, temp;
            mpu.getEvent(&a, &g, &temp);

            // Subtract the last reading:
            total = total - readings[readIndex];
            // Read from the sensor:
            readings[readIndex] = a.acceleration.z;
            // Add the reading to the total:
            total = total + readings[readIndex];
            // Advance to the next position in the array:
            readIndex = readIndex + 1;

            // If we're at the end of the array...
            if (readIndex >= numReadings) {
                // ...wrap around to the beginning:
                readIndex = 0;
            }

            // Calculate the average:
            average = total / numReadings;

            if (abs(average) > intenseMovementThreshold) {
                // Intense movement detected, send "reset" command
                pCharacteristic->setValue("reset");
                pCharacteristic->notify();
                Serial.println("Intense movement detected, sending reset command.");
            } else {
                //
