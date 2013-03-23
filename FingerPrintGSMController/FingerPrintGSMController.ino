#include <SoftwareSerial.h>


SoftwareSerial gsm(0,1);
SoftwareSerial fingerPrint(4,5);

byte fpShieldCommandPacket[24];
byte fpShieldResponsePacket[48];
boolean enrollIsActive = false;
boolean runOnce = true;
String gsmResponseMessage = "";
String userNumber = "09999969515";
String userPassword = "123456";
int smsCount = 35;

int starterRelay = 6;
int fpSwitch = 7;
int fpSwitchOn = 0;
int speedMeter = 8;
int leftSwitch = 9;
int rightSwitch = 10;

void setup() {
        Serial.begin(38400);
        gsm.begin(9600);
	fingerPrint.begin(115200);
        delay(2000);

        initializePin();
        initializeGSM();			
}

void loop() {       
   
        if(gsm.available()) {
                receiveGSMResponse();
                Serial.println("GSM Responding...");
                if(gsmResponseMessage.substring(2,7)=="+CMT:"){
                        readSMSCommand();                        
                        smsCount++;
                        if(smsCount >= 36) {
                                deleteAllSMS();
                        }
                }
                clearGsmResponseMessage();                
        }    
    
/*
                fpSwitchOn = digitalRead(fpSwitch);      
                
                if(fpSwitchOn) { 
                        if(runOnce) {
                                Serial.println("Read FingerPrint...");  
                                readFingerPrint();
                                runOnce = false;
                        }
                }
                
                else {
                        digitalWrite(starterRelay, LOW);            
                        runOnce = true;
                }
        
                if(fingerPrint.available()) {
                        receiveResponsePacket();
                        Serial.println("FP Responding...");
                       	if((fpShieldResponsePacket[30] == 20 || fpShieldResponsePacket[8] == 20)&& !enrollIsActive) {
                                Serial.println("Finger Match");
        		        digitalWrite(starterRelay, HIGH);
        	        }
        	        clearPacket(fpShieldResponsePacket);
                }         
*/             
}

void initializePin() {
        pinMode(starterRelay, OUTPUT);
        pinMode(fpSwitch, INPUT);
        pinMode(speedMeter, INPUT);
        pinMode(leftSwitch, INPUT);
        pinMode(rightSwitch, INPUT);
        
        digitalWrite(starterRelay, LOW);  
}

void initializeGSM() {
          sendATCommand("AT");
          delay(1000);
          sendATCommand("AT+CMGF=1");
          delay(1000);          
}

void gsmSMSListener() {
          receiveGSMResponse();
          if(gsmResponseMessage.substring(2,7)=="+CMT:"){
                    readSMSCommand();                        
                    smsCount++;
                    if(smsCount >= 36) {
                          deleteAllSMS();
                    }
          }
          clearGsmResponseMessage();   
}

void sendATCommand(String atCommand) {          
          gsm.println(atCommand);
}

void receiveGSMResponse() {
          while(gsm.available()) {
                  char incomingData = '\0';
                  incomingData = gsm.read(); 
                  gsmResponseMessage += incomingData;
          }         
}

void clearGsmResponseMessage() {
          gsmResponseMessage = String("");
}

void deleteAllSMS() {
          for(int i=1;i<=40;i++) {
                  String atCommand = "AT+CMGD=";
                  atCommand += i;
                  sendATCommand(atCommand);
                  delay(1000);
          }
}

void readSMSCommand() {
          int smsIndexLocation = 0;
          String command;
          String password;
          String sms;
          
          smsIndexLocation = gsmResponseMessage.indexOf("\"");
          sms = gsmResponseMessage.substring(smsIndexLocation+1);
          
          while(sms.indexOf("\"")>=0) {
                  smsIndexLocation = sms.indexOf("\"");
                  sms = sms.substring(smsIndexLocation+1);
          }
          
          sms = sms.substring(2);
          smsIndexLocation = sms.indexOf(32);
          command = sms.substring(0,smsIndexLocation);
          password = sms.substring(smsIndexLocation+1,smsIndexLocation+7);       
          
          if(command == "STOP" && password == userPassword) {
                  Serial.println("Stop");            
                  digitalWrite(starterRelay, LOW);
                  sendSMSAlert("Engine is stop.");          
          }
          if(command == "OVERRIDE" && password == userPassword) {
                  Serial.println("Override");            
                  digitalWrite(starterRelay, HIGH);                  
          }
          if(command == "CHANGE_ID" && password == userPassword) {
                  Serial.println("Change ID");
                  digitalWrite(starterRelay, LOW);           
                  deleteAllFingerPrint();
                  delay(2000);
                  enrollFingerPrint();
                  delay(2000);
                  readFingerPrint();                 
          }        
}

void sendSMSAlert(String message) {
          char ctl_z = 0x1A;
          String atCommand = "AT+CMGS= \"";
          
          atCommand += userNumber + "\"";
          sendATCommand(atCommand);
          delay(300);
          
          atCommand = message + ctl_z;
          sendATCommand(atCommand);
          delay(90);
}

void clearPacket(byte *packet) {
	for(int i=0; i<=50; i++) {
		packet[i] = 0x00;	
	}
}

void readFingerPrint() {
        enrollIsActive = false;
	clearPacket(fpShieldCommandPacket);
	fpShieldCommandPacket[0] = 0x55;
 	fpShieldCommandPacket[1] = 0xAA;
  	fpShieldCommandPacket[2] = 0x02;
  	fpShieldCommandPacket[3] = 0x01;

	sendCommandPacket();
        delay(500);       
}

void sendCommandPacket() {
	getCheckSum();
	fingerPrint.write(fpShieldCommandPacket,24);
}

void getCheckSum() {
	int checkSum = 0;
	
	for(int i=0; i<=21;i++) {
		checkSum += fpShieldCommandPacket[i];
	}
	fpShieldCommandPacket[22] = checkSum & 0xFF;
	fpShieldCommandPacket[23] = (checkSum - (checkSum & 0xFF))/256;
}

void receiveResponsePacket() {
	int i = 0;
	while(fingerPrint.available()) {
		fpShieldResponsePacket[i] = fingerPrint.read();
		i++;
	}
}

void deleteAllFingerPrint() {
        clearPacket(fpShieldCommandPacket);
        fpShieldCommandPacket[0] = 0x55;
        fpShieldCommandPacket[1] = 0xAA;
        fpShieldCommandPacket[2] = 0x06;  
        fpShieldCommandPacket[3] = 0x01;
        
        sendCommandPacket();
        delay(500);
}

void enrollFingerPrint() {
        enrollIsActive = true;
        clearPacket(fpShieldCommandPacket);
        fpShieldCommandPacket[0] = 0x55;
        fpShieldCommandPacket[1] = 0xAA;
        fpShieldCommandPacket[2] = 0x03;
        fpShieldCommandPacket[3] = 0x01;
        fpShieldCommandPacket[4] = 0x02;
        fpShieldCommandPacket[5] = 0x00;
        fpShieldCommandPacket[6] = 0x00;
        fpShieldCommandPacket[7] = 0x01;  
        
        sendCommandPacket();
        delay(500);
}

void initializeFingerPrint() {

}
