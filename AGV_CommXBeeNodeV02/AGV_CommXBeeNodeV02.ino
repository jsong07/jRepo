#include "RFID.h"
#include "XBeeNet.h"

//--------------- RFID Card Reader ---------------------
#define   CHK_BYTE1    0x04
#define   CHK_BYTE2    0x99
#define   IDX_CHK_BYTE1   1
#define   IDX_CHK_BYTE2   15 

RFID rfid;

const int bufferSize = 16;
byte rfidBuffer[bufferSize];


//--------------- XBee network Communication -----------
uint32_t CoordinatorAddressDH = 0;
uint32_t CoordinatorAddressDL = 0;

XBEENWK xbeeNwk(CoordinatorAddressDH, CoordinatorAddressDL);
bool txResult = true;

#define MAX_RETRIES   10
byte cntMaxRetries;

//-------------- Communicate with PLC ------------------
#define pinPLC  28

//-------------- Command -------------------------------
#define STATUS_WAIT   1
#define STATUS_WALK   2
#define STATUS_UNKOWN_CMD 3
#define STATUS_WAIT_YOUR_TURN 4

void setup() {
  // put your setup code here, to run once:
   Serial.begin(115200); // Initialize serial communications with the PC
   Serial2.begin(9600);                   //Communication with Xbee

   pinMode(pinPLC, OUTPUT);
   agvWalk();

   //initial RFID Reader
   rfid.initRFID_MFRC522();

   //initial Xbee
   xbeeNwk.setSerial(Serial2);
   xbeeNwk.initXBee();

   Serial.println("...Ready...");
}

void loop() {
  // put your main code here, to run repeatedly:
  int rxStatus = xbeeNwk.ReceiveData();
  if( rxStatus == XBEENWK::STATUS_RX_OK ){
    byte cmd = cmdCatagories(xbeeNwk.rxResultXbee.rx);
    if( cmd == STATUS_WAIT ){
      agvWait();
      Serial.println("Wait...");Serial.println();
    }else if(cmd == STATUS_WALK){
      agvWalk();
      Serial.println("Walk...");Serial.println();
    }else if(cmd == STATUS_WAIT_YOUR_TURN){
      rfid.clearUID();
      Serial.println("Wait, It isn't my turn.");Serial.println();
    }else{
      Serial.println("Unknown command...");Serial.println();
    }
  }

  if( rfid.readRFID_MFRC522(rfidBuffer) == RFID::STATUS_OK ){
    Serial.println("Show data from main loop:");
    rfid.dump_byte_array(rfidBuffer, bufferSize); Serial.println();
    Serial.println();

    //Check packet before transmit
    if( (rfidBuffer[IDX_CHK_BYTE1]==CHK_BYTE1) && (rfidBuffer[IDX_CHK_BYTE2]==CHK_BYTE2) ){
      //it should be wait/stop for the command from Section Control
      if(xbeeNwk.TransmitData(rfidBuffer) == XBEENWK::STATUS_TX_OK){
        txResult = true;
      }else{
        txResult = false;
        cntMaxRetries = MAX_RETRIES;
      }
    }else{
      Serial.println("Invalid RFID Card...");
    }
  }

  //repeat transmit the packet via xbee
  if(!txResult && (cntMaxRetries > 0)){
    //it should be wait/stop for the command from Section Control
    if(xbeeNwk.TransmitData(rfidBuffer) == XBEENWK::STATUS_TX_OK){
      txResult = true;
    }else{
      cntMaxRetries--;
    }
  }

}

void agvWait(){
  digitalWrite(pinPLC, LOW);
}

void agvWalk(){
  digitalWrite(pinPLC, HIGH);
}

byte cmdCatagories(ZBRxResponse rx){
  //to define the rx packet
  if( rx.getData(0)=='w' && rx.getData(1)=='a' && rx.getData(2)=='i' && rx.getData(3)=='t' ){
    return STATUS_WAIT;
    
  }else if( rx.getData(0)=='w' && rx.getData(1)=='a' && rx.getData(2)=='l' && rx.getData(3)=='k' ){
    return STATUS_WALK;
    
  }else if( rx.getData(0)=='w' && rx.getData(1)=='y' && rx.getData(2)=='t' && rx.getData(3)=='p' ){
    return STATUS_WAIT_YOUR_TURN;
    
  }else{
    Serial.println("Unknown command...");
    return STATUS_UNKOWN_CMD;
  }
}

