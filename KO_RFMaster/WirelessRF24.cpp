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
    //Serial.println("Initialize nRF24L01...");
}

void WirelessRF24::networkUpdate(){
    network.update();                  // Check the network regularly
}

bool WirelessRF24::dataAvailable(void* payload, uint16_t len){
    while ( network.available() ) {     // Is there anything ready for us?
        RF24NetworkHeader header;        // If so, grab it and print it out
        network.read(header, payload, len);
        //Serial.println("Received...");
        return true;
    }
    return false;
}

bool WirelessRF24::dataTransmit(const void* payload, uint16_t len){
    //Serial.print("Sending...");
    RF24NetworkHeader header(/*to node*/ other_node);
    //Serial.print("Size: "); Serial.println(sizeof(payload));

    return (network.write(header, payload, len) );
}

bool WirelessRF24::dataTransmit(void* payload, uint16_t len, uint8_t desAddr){
    //Serial.print("Sending...");
    RF24NetworkHeader header(/*to node*/ desAddr);

    return (network.write(header, payload, len ) );
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
