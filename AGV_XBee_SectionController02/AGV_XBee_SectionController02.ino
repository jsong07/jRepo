#include "XBeeNet.h"


//-======================= Communication with Zigbee ================================================
const int sizeTxComm = 4;     //Edit this to commmand size
uint8_t tx_comm[sizeTxComm];

#define RETRIES_NUM   3

XBEENWK xb;

//======================== Section Control ========================================
const int maxNumWaitingList  =  10;       //Edit this to Maximum Waiting List (Waiting + Owner) Number
const int maxNumSection  =  8;            //Edit this to change Maximum Section Number

unsigned long SectionStateMem[maxNumWaitingList][maxNumSection];
unsigned long LiveAGVMem[100];

String CommandStr = "";
char Input_State = 0;
char RFID_Report = true;
char Auto_Dump = true;
int MenuNum = 0;

//======================== SetUp =================================================
void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600);    //communicate with xbee
  
  CommandStr.reserve(40);

  //initial xbee communication
  xb.setSerial(Serial2);
  xb.initXBee();
  
  Serial.println("Section Controller Start up!");
  Serial.print("!! NOT exceed ");
  Serial.print(maxNumWaitingList);
  Serial.println(" AGVs at single section !!");
  Serial.println("Configure your terminal command sent with CR/LF.");
  Serial.println("");
  Serial.println("to show Menu. just press [Enter] key.");
  delay(100);

  //----------- Initialize List ----------------
  for (byte i = 0; i < maxNumWaitingList; i++)
    for (byte j = 0; j < maxNumSection; j++)
      SectionStateMem[i][j] = 0xFFFFFFFF;
  
  for (byte i = 0; i < 100; i++)
    LiveAGVMem[i] = 0xFFFFFFFF;

}

//=========================== Main Loop =======================================
void loop()
{
  byte rxStatus = xb.ReceiveData();
  if( rxStatus == XBEENWK::STATUS_RX_OK ){
    //Show data in the packet
    if (RFID_Report) showPacketData(xb.rxResultXbee.rx);
    byte cmd = packetCatagories(xb.rxResultXbee.rx);
  }
  
  SerialInput();
}

void showPacketData(ZBRxResponse rx){
  for (int i = 0; i < rx.getDataLength(); i++){
      if ((i == 2) || (i == 4) || (i == 8) || (i == 10))
        Serial.print(" ");
      if ((i == 2) || (i == 3))
        Serial.print((char)rx.getData(i));
      else
        Serial.print(rx.getData(i), HEX);
  }
  Serial.println("");
}

//================================= Process AGV Request Packet =====================================
byte packetCatagories(ZBRxResponse rx){
  unsigned int SectionID;

  if( rx.getData(0) == '%' ){
    
    if( rx.getData(1)=='C' && rx.getData(2)=='Q' ){
      
      Serial.println("Catch AGV searching");
      Response_AGV_Searching(rx.getRemoteAddress64().getMsb(), rx.getRemoteAddress64().getLsb());
      
    }else if( rx.getData(1)=='R' && rx.getData(2)=='S' ){
      
      Serial.println("Catch AGV Response");
      Serial.print("AGV: ");
      Serial.print(rx.getRemoteAddress64().getLsb(), HEX);
      Serial.println(" is now under control");
      LiveAGVMemStore(rx.getRemoteAddress64().getLsb());
      
    }
  }else if( rx.getData(2)=='S' && rx.getData(3)=='E' ){
    
    Serial.print("Exit Report: ");
    showPacketData(rx);
    Serial.println("Send back ");
    SectionID = (rx.getData(6) << 8) | rx.getData(7);

    SectionEndProc(rx.getRemoteAddress64().getLsb(), SectionID);
    
  }else if( rx.getData(2)=='S' && rx.getData(3)=='S' ){
    
    Serial.print("Entry Request: ");
    showPacketData(rx);
    SectionID = (rx.getData(6) << 8) | rx.getData(7);
    SectionStartProc(rx.getRemoteAddress64().getLsb(), SectionID);
    
  }else{
    Serial.println("Invalid Packet.");
  }
}
//=========================================================================
void Response_AGV_Searching(unsigned long AddrH, unsigned long AddrL)
{
  tx_comm[0] = '%';
  tx_comm[1] = 'R';
  tx_comm[2] = 'S';

  LiveAGVMemStore(AddrL);

  Serial.print("AGV: ");
  Serial.print(AddrL, HEX);
  Serial.println(" is now under control");

  //Retries to transmit data
  uint8_t retries = RETRIES_NUM;
  while(retries){
    if( xb.TransmitData(XBeeAddress64(AddrH, AddrL), tx_comm, sizeof(tx_comm)) == XBEENWK::STATUS_TX_OK ){
      retries = 0;
    }else{
      retries--;
    }
  }
  //xb.TransmitData(XBeeAddress64(AddrH, AddrL), tx_comm, sizeof(tx_comm));
}

//==========================================================================
void LiveAGVMemStore(unsigned long TargetAddr)
{
  char isStored = false;

  for (byte i = 0; i < 100; i++){
    if (LiveAGVMem[i] == TargetAddr){
      isStored = true;
      break;
    }
  }
  if (isStored == false){
    for (int i = 0; i < 100; i++){
      if (LiveAGVMem[i] == 0xFFFFFFFF){
        LiveAGVMem[i] = TargetAddr;
        break;
      }
    }
  }
}

//====================== Update Waiting List after pass through the Start Section ==============
void SectionStartProc(unsigned long TargetAddr, unsigned long SectionID)
{
  bool newAGV = true;
  SectionID--;

  //-------------- Check Multiple AGV Entry at the same Section ---------------------------
  for(int i=0; i<maxNumWaitingList; i++){
    if( SectionStateMem[i][SectionID] == TargetAddr ){
      Serial.println(" ");Serial.println(" "); Serial.print("Section#:");
      Serial.print(SectionID+1); Serial.print("  AGV:"); Serial.print(TargetAddr, HEX); Serial.println(" MultipleEntry");
      newAGV = false;
      break; 
    } 
  }

  //--------------- New AGV Enter to Section Start -------------------
  if(newAGV){
    if(SectionStateMem[0][SectionID] == 0xFFFFFFFF){
      
      SectionStateMem[0][SectionID] = TargetAddr;

      Serial.println(" 1st AGV Send back");
      Send_Command_AGV("walk", TargetAddr);

      Serial.println(" ");
      Serial.print("Section# :");
      Serial.print(SectionID+1);
      Serial.print("    AGV:");
      Serial.print(TargetAddr, HEX);
      Serial.println(" Entry");
      
    }else{      //---------------- Add to Waiting List ---------------
      int i;
      for(i=1; i<maxNumWaitingList; i++){
        
        if(SectionStateMem[i][SectionID] == 0xFFFFFFFF ){

          Send_Command_AGV("wait", TargetAddr);
          Serial.println(" "); Serial.print("Section#:");
          Serial.print(SectionID+1); Serial.print("  AGV:"); Serial.print(TargetAddr, HEX); Serial.println(" Waiting");
          SectionStateMem[i][SectionID] = TargetAddr;
          Serial.println("Waiting AGV Send back ");
          break;
            
        }
      }
      if(i == maxNumWaitingList){
        Serial.println(" "); Serial.println(""); 
        Serial.print("Warning: Section ");
        Serial.print(SectionID+1);
        Serial.println(" Exceed maximum waiting list.");
      }
    }
  }

  //Show Waiting List
  if (Auto_Dump)
    Dump_SectionStateMem();
}

//====================== Update Waiting List after pass through the End Section ==============
byte SectionEndProc(unsigned long MoveOutAddr, unsigned long SectionID)
{
  unsigned long TargetAddr;
  SectionID--;

  if( SectionStateMem[0][SectionID] != MoveOutAddr && MoveOutAddr!=0x00){
    Serial.print(MoveOutAddr, HEX);
    Serial.println(": It isn't your turn, please wait.");
    Send_Command_AGV("WYT", MoveOutAddr);
    return 1;
  }

  if(maxNumWaitingList > 1){
    //Update Waiting List
    for(int i=0; i<maxNumWaitingList-1; i++){
      SectionStateMem[i][SectionID] = SectionStateMem[i+1][SectionID];
    }
    SectionStateMem[maxNumWaitingList-1][SectionID] = 0xFFFFFFFF;
  }else{
    SectionStateMem[0][SectionID]= 0xFFFFFFFF;
  }

  //Move the next one
  if(SectionStateMem[0][SectionID] != 0xFFFFFFFF){
    Serial.println(" Next AGV Move... ");
    TargetAddr = SectionStateMem[0][SectionID];
    Send_Command_AGV("walk", TargetAddr);
    Serial.println(" "); Serial.print("Section#:");
    Serial.print(SectionID+1); Serial.print("  AGV:"); Serial.print(TargetAddr, HEX); Serial.println(" Entry");
  }else{
    Serial.println(" "); Serial.print("Section# : ");
    Serial.print(SectionID+1); Serial.println(" Clear");
  }

  //Show Waiting List
  if (Auto_Dump)
    Dump_SectionStateMem();

  return 0;
}

//============ Prepare Command Packet and target address node, then transmit it ==========
void Send_Command_AGV(String Command, unsigned long Target)
{
  if (Command == "wait"){
    tx_comm[0] = 'w'; tx_comm[1] = 'a'; tx_comm[2] = 'i'; tx_comm[3] = 't';
  }else if (Command == "walk"){
    tx_comm[0] = 'w'; tx_comm[1] = 'a'; tx_comm[2] = 'l'; tx_comm[3] = 'k';
  }else if (Command == "WYT"){
    tx_comm[0] = 'w'; tx_comm[1] = 'y'; tx_comm[2] = 't'; tx_comm[3] = 'p';
  }

  //Retries to transmit data
  uint8_t retries = RETRIES_NUM;
  while(retries){
    if( xb.TransmitData(XBeeAddress64(0x0013a200, Target), tx_comm, sizeof(tx_comm)) == XBEENWK::STATUS_TX_OK ){
      retries = 0;
    }else{
      retries--;
      Serial.print("Retry transmit:"); Serial.println(retries);
    }
  }
  //xb.TransmitData(XBeeAddress64(0x0013a200, Target), tx_comm, sizeof(tx_comm));

}

//=============================== Show AGV in each Section ==================================
void Dump_SectionStateMem()
{
  Serial.println("");
  Serial.println("SectionStateMem Dump");
  Serial.println("");

  Serial.print("Section No. ");
  for(int i=0; i< maxNumSection; i++){
    Serial.print("        ");
    Serial.print(i+1);
    Serial.print(", ");
    
  }
  Serial.println("");
  
  for (byte i = 0; i < maxNumWaitingList; i++){
    for (byte j = 0; j < maxNumSection; j++){
      if (j == 0){
        if (i == 0){  
          Serial.print("Owner        ");
        }else{
          Serial.print("Wait ");
          Serial.print(i);
          Serial.print("       ");
        }  
      }
      if (SectionStateMem[i][j] != 0xFFFFFFFF){
        Serial.print(SectionStateMem[i][j], HEX);
        Serial.print(",  ");
      } else{
        Serial.print("        ");
        Serial.print(",  "); 
      }
    }
    Serial.println(" ");
  }
  Serial.println("");
}

//========================== Check Commmand from Serial Port ================================
void SerialInput()
{
  String ExecCommand;
  if (Serial.available()){
    char inChar = (char)Serial.read();
    if ((inChar != '\n') && (inChar != '\r'))
      CommandStr += inChar;
    if (inChar == '\n'){
      MenuProcess(CommandStr);
      CommandStr = "";
    }
  }
}


//========================== Process Serial Command =======================================
void MenuProcess(String ExecCommand)
{
  Serial.println("");
  unsigned long TargetSectionID = 0;
  if (MenuNum == 3){
    if ((ExecCommand.length()) == 0){
      MenuNum = 0;
    } else{
      
      TargetSectionID = ExecCommand.toInt();
      if (TargetSectionID > 0 )
      {
        Serial.println(TargetSectionID);
        ForceSectionClear(0, TargetSectionID);
      }
      
    }
    
  }

  if ((ExecCommand.length()) == 0){
    Serial.println("MENU");
    Serial.println("1: Start/Stop showing RFID read report");
    Serial.println("2: Start/Stop auto memory dump");
    Serial.println("3: Force section clear");
    Serial.println("4: AGV XBee address correction");
    Serial.println("5: Dump Section state mem");
    Serial.println("6: Dump live AGV store mem");
    MenuNum = 0;
  }
  if (MenuNum == 0){
    if (ExecCommand == "1"){
      if (RFID_Report){
        RFID_Report = false;
        Serial.println("RFID read report stopped");
      } else{
        RFID_Report = true;
        Serial.println("RFID read report started");
      }
    }
    if (ExecCommand == "2"){
      if (Auto_Dump){
        Auto_Dump = false;
        Serial.println("Section state memory auto aump stopped");
      } else{
        Auto_Dump = true;
        Serial.println("Section state memory auto dump started");
      }
    }
    if (ExecCommand == "3"){
      MenuNum = 3;
      Dump_SectionStateMem();
      Serial.println("");
      Serial.println("Please input closed sectionID");
    }
    if (ExecCommand == "4"){
      Serial.println("Send Request...");

      for (byte i = 0; i < 100; i++){
        LiveAGVMem[i] = 0xFFFFFFFF;
      }

      tx_comm[0] = '%';
      tx_comm[1] = 'C';
      tx_comm[2] = 'Q';

      //Retries to transmit data
      uint8_t retries = RETRIES_NUM +3;
      while(retries){
        if( xb.TransmitData(XBeeAddress64(0x0, 0xFFFF), tx_comm, sizeof(tx_comm)) == XBEENWK::STATUS_TX_OK ){
          retries = 0;
        }else{
          retries--;
        }
      }
      //xb.TransmitData(XBeeAddress64(0x0, 0xFFFF), tx_comm, sizeof(tx_comm));

    }
    if (ExecCommand == "5"){
      Dump_SectionStateMem();
    }
    if (ExecCommand == "6"){
      Serial.print("LiveAGVs: ");
      for (byte i = 0; i < 100; i++){
        if (LiveAGVMem[i] != 0xFFFFFFFF){
          Serial.print(LiveAGVMem[i], HEX);
          Serial.print(',');
        }
      }
    }
  }
}

//=========================== Clear Specified Section =================================
void ForceSectionClear(unsigned long addr, unsigned long TargetSection)
{
  SectionEndProc(addr, TargetSection);
}

