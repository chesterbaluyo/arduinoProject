#include <SoftwareSerial.h>


SoftwareSerial mySerial(2,3);
byte commandPacket[24] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
byte responsePacket[48] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
int checkSum;
int counter = 0;
char option;

void setup() {

  Serial.begin(115200);
  Serial.println("start fingertest");
    
  mySerial.begin(115200);  
}

void loop() { 
  if(Serial.available()){
    Serial.print("Select Option:");
    Serial.print("[1]Delete All");
    Serial.print("[2]Enroll FingerPrint");
    Serial.print("[3]Read Finger Print");
    option = Serial.read();
    Serial.println(option);
    if(option == '1'){
    deleteAllFingerPrint();  
    }
    if(option == '2'){
    enrollFingerPrint();
    }
    if(option == '3'){
    readFingerPrint();
    }      
  }
  getResponse();
}

void enrollFingerPrint() {
  emptyBytes();
  //set command
  commandPacket[0] = 0x55;
  commandPacket[1] = 0xAA;
  commandPacket[2] = 0x03;
  commandPacket[3] = 0x01;
  commandPacket[4] = 0x02;
  commandPacket[5] = 0x00;
  commandPacket[6] = 0x00;
  commandPacket[7] = 0x01;  
  sendCommandPacket();
  delay(500);
  
}

void readFingerPrint() {
  emptyBytes();
  commandPacket[0] = 0x55;
  commandPacket[1] = 0xAA;
  commandPacket[2] = 0x02;
  commandPacket[3] = 0x01;
  sendCommandPacket();
  delay(500); 
}

void deleteAllFingerPrint() {
  emptyBytes();
  commandPacket[0] = 0x55;
  commandPacket[1] = 0xAA;
  commandPacket[2] = 0x06;  
  commandPacket[3] = 0x01;
  sendCommandPacket();
  delay(500);  
  Serial.println("All users deleted.");  
}

void emptyBytes() {
  int i;
  for(i=0;i<=23;i++){
    commandPacket[i]=0x00;
  }
}

void getCheckSum() {
  checkSum = 0;
  int i;
  
  for(i=0; i<=21;i++){
   checkSum += commandPacket[i]; 
  } 
  commandPacket[22] = checkSum & 0xFF;
  commandPacket[23] = (checkSum - (checkSum & 0xFF))/256;
}

void sendCommandPacket() {
  getCheckSum();
  mySerial.write(commandPacket,24);
}

void getResponse() {
    if(mySerial.available()>0) {     
//        Serial.println("----------------");    
        int responseValue = 0;
        responsePacket[counter] = mySerial.read();
//        Serial.print(responsePacket[counter]);
//        Serial.print(" counter: ");
//        Serial.println(counter);
 
        counter++;
        if(counter >=48){
          counter = 0;
        }
        responseValue = responsePacket[30];
        if(responseValue == 20){
          Serial.println("This is a test.........");      
        }          
    
    }
  
}

