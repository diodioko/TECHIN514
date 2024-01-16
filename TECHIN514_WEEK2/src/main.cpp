#include <Arduino.h>

const int analogPin = 21; // Change to your ESP32's analog pin number

void setup() {
  Serial.begin(115200); // Start the serial communication
  pinMode(analogPin, INPUT); // Set the analog pin as input
}

void loop() {
  int sensorValue = analogRead(analogPin); // Read the analog value
  float voltage = sensorValue * (3.3 / 4095.0); // Convert to voltage (assuming 3.3V reference and 12-bit resolution)
  Serial.println(voltage); // Print the voltage to the serial monitor
  delay(1000); // Wait for a second
}
