#include "WirelessRF24.h"

#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

//define nRF24L01 pin (CE,CSN pin)
#define     pinCE       9
#define     pinCSN      10

RF24 radio(pinCE, pinCSN);
RF24Network network(radio);

WirelessRF24::WirelessRF24(){

}
WirelessRF24::WirelessRF24(uint8_t channel, uint16_t myAddr){
    _channel = channel;
    this_node = myAddr;
}

void WirelessRF24::initRF24L01(){
    SPI.begin();
    radio.begin();
    network.begin( _channel, /*node address*/ this_node);
    Serial.println("Initialize nRF24L01...");
}

void WirelessRF24::networkUpdate(){
    network.update();                  // Check the network regularly
}

bool WirelessRF24::dataAvailable(){
    while ( network.available() ) {     // Is there anything ready for us?
        RF24NetworkHeader header;        // If so, grab it and print it out
        //payload_t payload;
        network.read(header,&payload,sizeof(payload));
        Serial.println("Received...");
        return true;
    }
    return false;
}

bool WirelessRF24::dataTrasmit(payload_t payload){
    Serial.print("Sending...");
    RF24NetworkHeader header(/*to node*/ other_node);

    return (network.write(header,&payload,sizeof(payload)) );
}

bool WirelessRF24::dataTrasmit(payload_t payload, uint8_t desAddr){
    Serial.print("Sending...");
    RF24NetworkHeader header(/*to node*/ desAddr);

    return (network.write(header,&payload,sizeof(payload)) );
}

void WirelessRF24::setChannel(uint8_t channel){
    _channel = channel;
}

void WirelessRF24::setMyAddress(uint16_t address){
    this_node = address;
}

void WirelessRF24::setDestinationAddress(uint16_t address){
    other_node = address;
}
