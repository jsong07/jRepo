#include "XBeeNet.h"


// create the XBee object
XBee xbee = XBee();

// SH + SL Address of receiving XBee
XBeeAddress64 addr64;
ZBTxRequest zbTx;
ZBTxStatusResponse txStatus;

// create reusable response objects for responses we expect to handle 
ZBRxResponse rx = ZBRxResponse();
ModemStatusResponse msr = ModemStatusResponse();

//Constructor
XBEENWK::XBEENWK(){
  
}

//Constructor Xbee destination address
XBEENWK::XBEENWK(uint32_t addressDH, uint32_t addressDL){
  _addressDH = addressDH;
  _addressDL = addressDL;
}

XBEENWK::XBEENWK(uint32_t addressDH, uint32_t addressDL, Stream &serial){
  _addressDH = addressDH;
  _addressDL = addressDL;
  _serial = &serial;
}

void XBEENWK::flashLed(int pin, int times, int wait) {

  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(wait);
    digitalWrite(pin, LOW);

    if (i + 1 < times) {
      delay(wait);
    }
  }
}

void XBEENWK::setSerial(Stream &serial) {
  _serial = &serial;
}

void XBEENWK::initXBee() {
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);

  xbee.setSerial(*_serial);

  addr64 = XBeeAddress64(_addressDH, _addressDL);
  zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
  txStatus = ZBTxStatusResponse();

  rx = ZBRxResponse();
  msr = ModemStatusResponse();
}

int XBEENWK::TransmitData(uint8_t *data) {   

  //preparing payload to transmit
  for(int i=0; i<payloadSize; i++){
    payload[i] = data[i];
    //Serial.println(payload[i], HEX);
  }

  xbee.send(zbTx);

  // flash TX indicator
  flashLed(statusLed, 1, 100);

  // after sending a tx request, we expect a status response
  // wait up to half second for the status response
  if (xbee.readPacket(200)) {
    // got a response!

    // should be a znet tx status            	
    if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
      xbee.getResponse().getZBTxStatusResponse(txStatus);

      // get the delivery status, the fifth byte
      if (txStatus.getDeliveryStatus() == SUCCESS) {
        
        // success.  time to celebrate
        flashLed(statusLed, 5, 50);
        Serial.println("Tx Success...");
        return STATUS_TX_OK;
        
      } else {
        // the remote XBee did not receive our packet. is it powered on?
        flashLed(errorLed, 3, 500);
        Serial.println("Tx Failed...");
        return STATUS_TX_FAIL;
      }
    }
  } else if (xbee.getResponse().isError()) {
    Serial.print("Error reading packet.  Error code: ");  
    Serial.println(xbee.getResponse().getErrorCode());
    return STATUS_ERROR_PACKET;
  } else {
    // local XBee did not provide a timely TX Status Response -- should not happen
    flashLed(errorLed, 2, 50);
    return STATUS_XBEE_NOSTATUS;
  }

}

int XBEENWK::TransmitData(XBeeAddress64 destAddress, uint8_t *data, uint8_t length) {   

  addr64 = destAddress;
  
  //preparing payload to transmit
  for(int i=0; i<payloadSize; i++)
    payload[i] = 0;
  
  for(int i=0; i<length; i++){
    payload[i] = data[i];
    //Serial.println(payload[i], HEX);
  }

  zbTx = ZBTxRequest(addr64, payload, sizeof(payload));
  xbee.send(zbTx);

  // flash TX indicator
  flashLed(statusLed, 1, 100);

  // after sending a tx request, we expect a status response
  // wait up to half second for the status response
  if (xbee.readPacket(200)) {
    // got a response!

    // should be a znet tx status              
    if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
      xbee.getResponse().getZBTxStatusResponse(txStatus);

      // get the delivery status, the fifth byte
      if (txStatus.getDeliveryStatus() == SUCCESS) {
        
        // success.  time to celebrate
        flashLed(statusLed, 5, 50);
        Serial.println("Tx Success...");
        return STATUS_TX_OK;
        
      } else {
        // the remote XBee did not receive our packet. is it powered on?
        flashLed(errorLed, 3, 500);
        Serial.println("Tx Failed...");
        return STATUS_TX_FAIL;
      }
    }
  } else if (xbee.getResponse().isError()) {
    Serial.print("Error reading packet.  Error code: ");  
    Serial.println(xbee.getResponse().getErrorCode());
    return STATUS_ERROR_PACKET;
  } else {
    // local XBee did not provide a timely TX Status Response -- should not happen
    flashLed(errorLed, 2, 50);
    return STATUS_XBEE_NOSTATUS;
  }

}

byte XBEENWK::ReceiveData() {
    
    xbee.readPacket();
    
    if (xbee.getResponse().isAvailable()) {
      // got something
      
      if (xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
        // got a zb rx packet
        
        // now fill our zb rx class
        xbee.getResponse().getZBRxResponse(rx);
            
        if (rx.getOption() == ZB_PACKET_ACKNOWLEDGED) {
            // the sender got an ACK
            flashLed(statusLed, 10, 10);
        } else {
            // we got it (obviously) but sender didn't get an ACK
            flashLed(errorLed, 2, 20);
        }
        rxResultXbee.rx = rx;
        Serial.println("Got packet!!");

        return STATUS_RX_OK;
        
      } else if (xbee.getResponse().getApiId() == MODEM_STATUS_RESPONSE) {
        xbee.getResponse().getModemStatusResponse(msr);
        // the local XBee sends this response on certain events, like association/dissociation
        
        if (msr.getStatus() == ASSOCIATED) {
          // yay this is great.  flash led
          flashLed(statusLed, 10, 10);
        } else if (msr.getStatus() == DISASSOCIATED) {
          // this is awful.. flash led to show our discontent
          flashLed(errorLed, 10, 10);
        } else {
          // another status
          flashLed(statusLed, 5, 10);
        }
        Serial.println("got modem response!!");

        return STATUS_MODEM_RSP;
        
      } else {
        // not something we were expecting
        flashLed(errorLed, 1, 25);    
        return STATUS_UNEXPECTED;
      }
    } else if (xbee.getResponse().isError()) {
      Serial.print("Error reading packet.  Error code: ");  
      Serial.println(xbee.getResponse().getErrorCode());

      return STATUS_READ_ERR;
    }

    return STATUS_NO_PACKET;
}


