#include <SoftwareSerial.h>


SoftwareSerial gsm(2,3);

byte fpShieldCommandPacket[24];
byte fpShieldResponsePacket[48];
boolean enrollIsActive = false;
boolean deleteIsActive = false;
boolean deleteAllSMSisActive = false;
String gsmResponseMessage = "";
String userNumber = "09352983630";
String userPassword = "123456";
int smsCount = 0;

int starterRelay = 6;
int fpSwitch = 7;
int fpSwitchOn = 0;
int speedMeter = 8;
int leftSwitch = 9;
int rightSwitch = 10;

void setup() {
        gsm.begin(9600);
	Serial.begin(115200);

        initializePin();
        delay(3000);        
        initializeGSM();
        readFingerPrint();        			
}

void loop() { 
        if(!initializeFingerPrint()) {
                if(enrollIsActive) {
                        delay(5000);
                        digitalWrite(starterRelay, HIGH);
                        enrollFingerPrint();
                        enrollIsActive = false;                
                }          
                if(deleteIsActive) {
                        deleteAllFingerPrint();                  
                        deleteIsActive = false;
                        enrollIsActive = true;
                }                
        }
     
        if(deleteAllSMSisActive) {
                deleteAllSMS();
                deleteAllSMSisActive = false;
        }
        
        gsm.listen();
        gsmSMSListener();        
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
          receiveGSMResponse();
          clearGsmResponseMessage();
          sendATCommand("AT+CMGF=1");
          delay(1000);
          receiveGSMResponse();
          clearGsmResponseMessage();          
}

void gsmSMSListener() {
                
          if(receiveGSMResponse()) {
                Serial.print("Response:---");
                Serial.println(gsmResponseMessage);
                if(gsmResponseMessage.substring(2,6)=="+CMT"){
                        readSMSCommand();                        
                        smsCount++;
                        if(smsCount >= 15) {
                                deleteAllSMS();
                                smsCount = 0;
                        }
                }
                clearGsmResponseMessage();    
          }      
}

void sendATCommand(String atCommand) {          
          gsm.println(atCommand);
}

boolean receiveGSMResponse() {
          boolean isAvailable = false;
          
          while(gsm.available()>0) {
                  char incomingData = '\0';
                  incomingData = gsm.read(); 
                  gsmResponseMessage += incomingData;
                  isAvailable = true;
          }         
          return isAvailable;
}

void clearGsmResponseMessage() {
          gsmResponseMessage = String("");
}

void deleteAllSMS() {
          for(int i=1;i<=15;i++) {
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
                  digitalWrite(starterRelay, LOW);
                  sendSMSAlert("Engine is stop.");          
          }
          if(command == "OVERRIDE" && password == userPassword) {
                  digitalWrite(starterRelay, HIGH);
                  sendSMSAlert("Engine Start.");                  
          }
          if(command == "RENEW" && password == userPassword) {
                  digitalWrite(starterRelay, LOW);           
                  deleteIsActive = true;                  
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

boolean initializeFingerPrint() {
        boolean isActive = false;
        if(receiveResponsePacket()) {  
                    if((fpShieldResponsePacket[9] == 1)&& !enrollIsActive) {
         	            digitalWrite(starterRelay, HIGH);
        	    }
                    deleteAllSMSisActive = true;        
        	    clearPacket(fpShieldResponsePacket);
                    isActive = true;
        }      
        return isActive;
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
	Serial.write(fpShieldCommandPacket,24);
}

void getCheckSum() {
	int checkSum = 0;
	
	for(int i=0; i<=21;i++) {
		checkSum += fpShieldCommandPacket[i];
	}
	fpShieldCommandPacket[22] = checkSum & 0xFF;
	fpShieldCommandPacket[23] = (checkSum - (checkSum & 0xFF))/256;
}

boolean receiveResponsePacket() {
        boolean isAvailable = false;
	int i = 0;
	while(Serial.available()>0) {
		fpShieldResponsePacket[i] = Serial.read();
                Serial.print(i);
                Serial.print("-----");
                Serial.println(fpShieldResponsePacket[i]);
		i++;
                isAvailable = true;
	}
        return isAvailable;
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
