#include <SoftwareSerial.h>
#include <DTMF.h>

/* FingerPrint Shield */ 
byte fpShieldCommandPacket[24];
byte fpShieldResponsePacket[48];
boolean enrollIsActive = false;
boolean deleteIsActive = false;

/* GSM Shield */
SoftwareSerial gsm(2,3);
boolean deleteAllSMSisActive = false;
String gsmResponseMessage = "";
String userNumber = "09352983630";
String userPassword = "123456";
int smsCount = 0;

/* DTMF Decoder */
float n = 128.0;
float samplingRate = 8926.0;
DTMF dtmf = DTMF(n, samplingRate);
int noCharCount = 0;
float dMags[8];

/* Sensors and Switches */
int dtmfSensor = A0;
int starterRelay = 6;
boolean starterRelayIsOff = true;
int engineSwitch = 7;
int speedMeter = 8;
int leftSwitch = 9;
int rightSwitch = 10;

void setup() {
        gsm.begin(9600);
	Serial.begin(115200);

        initializePin();
        delay(3000);        
        initializeGSM();
        delay(500);
        setSMSMessageFormat();
        delay(500);
        enableAutomaticAnswer();
        delay(500);
        enableLoudSpeaker();
        delay(500);
        setLoudnessToMax();
        delay(500);
}

void loop() {
        if(digitalRead(engineSwitch)) {
                //TODO Test if condition should runs once          
                Serial.println("Should see this once.");
                if(starterRelayIsOff) {
                        Serial.print("Scanning Finger Print: ");
                        scanFingerPrint();         
                }                
        } else {
                if(checkFingerPrint() && starterRelayIsOff) {
                        Serial.println("PASSED");
                        digitalWrite(starterRelay, HIGH);
                }
                
                if(!checkFingerPrint()) {
                        if(enrollIsActive) {
                                delay(5000);
                                digitalWrite(starterRelay, HIGH);
                                starterRelayIsOff = false;
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
        
        //TODO if incoming call is available do readDTMF().
        readDTMF();        
}

void initializePin() {
        Serial.print("Initialize Sensors and Switches: ");
        pinMode(starterRelay, OUTPUT);
        pinMode(engineSwitch, INPUT);
        pinMode(speedMeter, INPUT);
        pinMode(leftSwitch, INPUT);
        pinMode(rightSwitch, INPUT);
        
        digitalWrite(starterRelay, LOW);  
        Serial.println("Done");        
}

void initializeGSM() {
        Serial.print("Initialize GSM: ");  
        sendATCommand("AT");
        receiveGSMResponse();
        clearGsmResponseMessage();                    
}

void setSMSMessageFormat() {
        Serial.print("Set SMS message format: ");  
        sendATCommand("AT+CMGF=1");
        receiveGSMResponse();
        clearGsmResponseMessage();
}

void enableAutomaticAnswer() {
        Serial.print("Enable automatic answering: ");  
        sendATCommand("ATS0=1");
        receiveGSMResponse();
        clearGsmResponseMessage();
}

void enableLoudSpeaker() {
        Serial.print("Loud speaker mode: ");   
        sendATCommand("ATM9"); 
        receiveGSMResponse();
        clearGsmResponseMessage();
}

void setLoudnessToMax() {
        Serial.print("Set maximum loudness: ");   
        sendATCommand("ATL9");
        receiveGSMResponse();
        clearGsmResponseMessage();          
}

void gsmSMSListener() {
        if(receiveGSMResponse()) {
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
 
        if(isAvailable) {
                Serial.println(gsmResponseMessage);          
        }
        else {
                Serial.println("*** GSM connection failed! ***");
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
                starterRelayIsOff = false;
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

boolean checkFingerPrint() {
        receiveResponsePacket();  
        if((fpShieldResponsePacket[9] == 1)) { 
                starterRelayIsOff = false;
                clearPacket(fpShieldResponsePacket);
                return true;              
        }        
        Serial.println("FAILED. Please try again.");
        clearPacket(fpShieldResponsePacket);
        return false;
  }
  
  void clearPacket(byte *packet) {
  	for(int i=0; i<=50; i++) {
  		packet[i] = 0x00;	
  	}
}

void scanFingerPrint() {
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

void readDTMF() {
        Serial.print("DTMF: ");
        char thisChar;
        dtmf.sample(dtmfSensor);
        dtmf.detect(dMags, 506);
        thisChar = dtmf.button(dMags, 1800.);
        if(thisChar) {
                Serial.print(thisChar);
                noCharCount = 0;
        } else {
                if(++noCharCount == 50) {
                        Serial.println("");
                }
                if(noCharCount > 30000) {
                        noCharCount = 51;
                }
        }
}
