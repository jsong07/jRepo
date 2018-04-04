#include "RFID.h"

#define RST_PIN         49           // Configurable, see your schemetic (MFRC522)
#define SS_PIN          53          // Configurable, see your schemetic (MFRC522)

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;


//============= Initialize MF_RC522 RFID Card ==================
void RFID::initRFID_MFRC522() {
  // put your setup code here, to run once:
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    
    Serial.println(F("Initial MF_RC522..."));

}

//============== Read data from the RFID card and check double read ============
// Return - 0 if OK
//        - 1 if read RFID not completed
//        - 2 if No compatible card
//        - 3 Authentication failed
//------------------------------------------------------------------------------
byte RFID::readRFID_MFRC522(byte *data) {
  
    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return STATUS_ERROR;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return STATUS_ERROR;

    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    
    //Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    //Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("This sample only works with MIFARE Classic cards."));
        return STATUS_NOCOMPAT;
    }

    // In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    byte sector         = 1;
    byte blockAddr      = 4;
    byte trailerBlock   = 7;
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);

    // Authenticate using key A
    //Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return STATUS_AUTHEN_FAIL;
    }

    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return STATUS_READ_FAIL;
    }
    Serial.print(F("RFID Data ")); /*Serial.print(blockAddr);*/ Serial.print(F(":"));
    dump_byte_array(buffer, rfDataSize); Serial.println();

    memcpy(data, buffer, rfDataSize);

    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();

    //Check repeat read RFID
    if( memcmp(uid, mfrc522.uid.uidByte, sizeof(uid)) == 0 ){
      Serial.println("Ignore this card(same UID...)");Serial.println();
      return STATUS_SAME_UID;
    }else{
      memcpy(uid, mfrc522.uid.uidByte, sizeof(uid));
    }
    
    return STATUS_OK;
}


/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void RFID::dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void RFID::clearUID(){

  for(int i=0;i<sizeof(uid); i++)
    uid[i] = 0;
}

