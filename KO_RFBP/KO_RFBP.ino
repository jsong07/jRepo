#include "WirelessRF24.h"

const uint8_t channel = 90;   //RF communication channel
const uint16_t myAddr = 01;   //Device address
const uint16_t desAddr = 00;  //Destination address

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

const unsigned long interval = 2000; //ms  // How often to send 'hello world to the other unit

unsigned long last_sent;             // When did we last send?
unsigned long packets_sent;          // How many have we sent already
unsigned char cnt = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //initial RF module
  wireless.initRF24L01();
  wireless.setDestinationAddress(desAddr);
  Serial.println("Initialize nRF24L01...");
}

void loop() {
  // put your main code here, to run repeatedly:
  wireless.networkUpdate();

  unsigned long now = millis();              // If it's time to send a message, send it!
  if ( now - last_sent >= interval  )
  {
    last_sent = now;

    payload_t payload;
    for(int i=0;i<dataLength-1;i++)
      payload.data[i] = i;
    
    payload.data[dataLength-1] = ++cnt;
    //transmit data
    if( wireless.dataTransmit(&payload, sizeof(payload)) ){
      Serial.println("Tx Success...");
    }else{
      Serial.println("Tx Failed...");
    }
  }
}
