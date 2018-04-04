#ifndef RFID_h
#define RFID_h


#include <SPI.h>
#include <MFRC522.h>



class RFID {

  public:
    
    static const byte rfDataSize = 16;
    byte uid[4];

    enum StatusCode {
      STATUS_OK = 0,
      STATUS_ERROR = 1,
      STATUS_NOCOMPAT = 2,
      STATUS_AUTHEN_FAIL = 3,
      STATUS_READ_FAIL = 4,
      STATUS_SAME_UID = 5
    };
    
    
    void initRFID_MFRC522();
    byte readRFID_MFRC522(byte *data) ;
    void clearUID();

    // Helper routine to dump a byte array as hex values to Serial.
    void dump_byte_array(byte *buffer, byte bufferSize);

};

#endif


