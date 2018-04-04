#ifndef __WIRELESSRF24_H__
#define __WIRELESSRF24_H__

#include <Arduino.h>

class WirelessRF24 {

    public:

        uint16_t this_node;    // Address of our node in Octal format ( 04,031, etc)
        uint16_t other_node;   // Address of the other node in Octal format

        uint8_t _channel = 90;

        // struct payload_t {                 // Structure of our payload
        //     unsigned int sys;
        //     unsigned int dia;
        //     unsigned int pulse;
        // };
        // payload_t payload;

        WirelessRF24();
        WirelessRF24(uint8_t channel, uint16_t myAddr);

        void initRF24L01();
        void networkUpdate();
        bool dataAvailable(void* payload, uint16_t len);
        bool dataTransmit(const void* payload, uint16_t len);
        bool dataTransmit(void* payload, uint16_t len, uint8_t desAddr);
        void setChannel(uint8_t channel);
        void setMyAddress(uint16_t address);
        void setDestinationAddress(uint16_t address);
};


#endif // __WIRELESSRF24_H__