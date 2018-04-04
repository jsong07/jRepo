#include "WirelessRF24.h"

const uint8_t channel = 90;     // RF communication channel
const uint16_t myAddr = 00;     //Device address (00 is parent)

WirelessRF24 wireless(channel, myAddr);   //Create RF object

// struct payload_t {                 // Structure of our payload
// unsigned int sys;
// unsigned int dia;
// unsigned int pulse;
// };
const int dataLength = 20;            // Structure of our payload
struct payload_t {
  unsigned char data[dataLength];
};

payload_t payload;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  //initial RF module
  wireless.initRF24L01();
  Serial.println("Initialize nRF24L01...");
}

void loop() {
  // put your main code here, to run repeatedly:
  wireless.networkUpdate();
  //Read data if available
  if(wireless.dataAvailable(&payload, sizeof(payload))){
    Serial.println("Received...");
    // Serial.print("Sys:");
    // Serial.print(payload.sys);
    // Serial.print(" Dia:");
    // Serial.println(payload.dia);

    //display received packet
    for(int i=0;i<dataLength;i++){
        Serial.print(payload.data[i]);
        Serial.print(" ");
    }
    Serial.println("");
  }

}
