#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(115200);
  Serial.println("I2C Scanner\r\n");
}

void loop() {
  byte address;
  Serial.println("Scanning...");
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte status = Wire.endTransmission();
    if (status == 0) {
      Serial.print("I2C device found at address 0x"); 
      Serial.println(address, HEX);
    } else if (status == 4) {
      Serial.print("Unknown error at address "); 
      Serial.println(address, HEX);
    }
  }
}
