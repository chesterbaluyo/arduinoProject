#include <SoftwareSerial.h>

SoftwareSerial mySerial(2,3);
byte commandPacket[24] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
byte responsePacket[48] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
int checkSum;
int counter = 0;
char option;

char Rx_data[50];
unsigned char Rx_index = 0;
int i = 0;
char msg[160];
char readMsg[500];
int sig;

void setup() {
  Serial.begin(38400);
    
  initGSM();
  readMessages();
  mySerial.begin(115200);  
  readFingerPrint();
}

void loop() {
  getResponse();
}

void send_msg(char *number, char *msg)
{
  char at_cmgs_cmd[30] = {'\0'};
  char msg1[160] = {'\0'};
  char ctl_z = 0x1A; 

  sprintf(msg1, "%s%c", msg, ctl_z);
  sprintf(at_cmgs_cmd, "AT+CMGS=\"%s\"\r\n",number);
  
  sendGSM(at_cmgs_cmd);
  delay(100);
  delay(100);
  delay(100);
  sendGSM(msg1);
  delay(90);  
}    

void sendGSM(char *string){
  Serial.write(string);
  delay(90);
}

void clearString(char *strArray) {
  int j;
  for (j = 100; j > 0; j--)
    strArray[j] = 0x00;
}

void send_cmd(char *at_cmd, char clr){
  char *stat = '\0';
  while(!stat){
    sendGSM(at_cmd);
    delay(90);
    readSerialString(Rx_data);
    stat = strstr(Rx_data, "OK");
  }
  if (clr){
    clearString(Rx_data);
    delay(200);
    stat = '\0';
  }
}

void initGSM(){
  
  send_cmd("AT\r\n",1);							
  send_cmd("AT+CMGF=1\r\n",1);			
  
  Serial.println("Success");
	
  delay(1000);
  delay(1000);
  delay(1000);
}

void readSerialString (char *strArray) {
  
  if(!Serial.available()) {
    return;
  }
  
  while(Serial.available()) {
    strArray[i] = Serial.read();
    i++;
  }
}

void readMessages() {
  char *stat = '\0';
  char msg1[160] = {'\0'};
  char *p_char;
  char *p_char1;
  char number[20];
  sprintf(msg1,"AT+CMGL=\"%s\"\r\n","ALL");  
  while(!stat){
    sendGSM(msg1);
    delay(100);
    readSerialString(readMsg);
    Serial.println("---------");
    Serial.println(readMsg);
    
    stat = strstr(readMsg, "Success");
  }
  p_char = strchr(readMsg,',');
  p_char1 = p_char+2;
  p_char = strchr((char *)(p_char1),'"');
  strcpy(number,(char *)(p_char1));
  Serial.print("this is the: ");
  Serial.print(number);
  deleteAllSMS();
}

void deleteAllSMS() {
  int index;
  char at_cmd[50] = {'\0'};  
  for(index=0;index<=60;index++)
  {
    sprintf(at_cmd,"AT+CMGD=\"%i\"\r\n",index);
    sendGSM(at_cmd);
    delay(1000);
  }
}

void readFingerPrint() {
  emptyBytes();
  commandPacket[0] = 0x55;
  commandPacket[1] = 0xAA;
  commandPacket[2] = 0x02;
  commandPacket[3] = 0x01;
  sendCommandPacket();
  delay(500);
   Serial.println("Enter FingerPrint"); 
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
        int responseValue;
        responsePacket[counter] = mySerial.read();
 
        counter++;
        if(counter >=48){
          counter = 0;
        }
        responseValue = responsePacket[30];
        if(responseValue == 20){
          Serial.println("Finger Match. Sending Text....");
          send_msg("09991165260", "Engine Start");
        }          
    
    }
  
}
