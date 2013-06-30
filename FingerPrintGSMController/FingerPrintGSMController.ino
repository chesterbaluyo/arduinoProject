#include <SoftwareSerial.h>
#include <DTMF.h>

/* FingerPrint Shield */ 
byte fpShieldCommandPacket[24];
byte fpShieldResponsePacket[48];
boolean activeAddUserFingerPrint = false;
boolean activeDeleteUserFingerPrint = false;

/* GSM Shield */
SoftwareSerial gsm(2,3);
//TODO get userNumber on sim phonebook
String userNumber = "09352983630";
String userPassword = "123456";

/* DTMF Decoder */
float n = 128.0;
float samplingRate = 8926.0;
DTMF dtmf = DTMF(n, samplingRate);
float dMags[8];

/* Sensors and Switches */
int dtmfSensor = A0;
int starterRelay = 6;
boolean starterRelayIsOff = true;
int ignitionSwitch = 7;
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
        deleteAllSMS();
        delay(500);
}

void loop() {
        if(digitalRead(ignitionSwitch)) {
                //TODO Test if condition should runs once          
                Serial.println("Should see this once.");
                if(starterRelayIsOff && (!activeAddUserFingerPrint || !activeDeleteUserFingerPrint)) {
                        scanFingerPrint();                  
                }                
        } else {
                checkFingerPrint();
                //TODO this should be inside readSMSCommand since it completes communication with the finger print shield. 
                if(activeAddUserFingerPrint && starterRelayIsOff) {
                        addUserFingerPrint();                                                
                        activeAddUserFingerPrint = false;                
                }    
                
                if(activeDeleteUserFingerPrint && starterRelayIsOff) {
                        deleteAllFingerPrint();
                        activeDeleteUserFingerPrint = false;
                        activeAddUserFingerPrint = true;
                }                             
                
                gsm.listen();
                gsmCallAndSMSListener();
                getDirection();              
        }          
}

void initializePin() {
        Serial.print("Initialize Sensors and Switches: ");
        pinMode(starterRelay, OUTPUT);
        pinMode(ignitionSwitch, INPUT);
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
}

void setSMSMessageFormat() {
        Serial.print("Set SMS message format: ");  
        sendATCommand("AT+CMGF=1");
        receiveGSMResponse();
}

void enableAutomaticAnswer() {
        Serial.print("Enable automatic answering: ");  
        sendATCommand("ATS0=1");
        receiveGSMResponse();
}

void enableLoudSpeaker() {
        Serial.print("Loud speaker mode: ");   
        sendATCommand("ATM9"); 
        receiveGSMResponse();
}

void setLoudnessToMax() {
        Serial.print("Set maximum loudness: ");   
        sendATCommand("ATL9");
        receiveGSMResponse();          
}

void gsmCallAndSMSListener() {  
        String gsmResponseMessage = receiveGSMResponse();
        
        if(gsmResponseMessage.substring(2,6)=="+CMT"){
                readSMSCommand(gsmResponseMessage);                        
                deleteAllSMS();                   
        }
        else if (gsmResponseMessage.substring(2,6)=="+CTI") {
        //TODO check if correct gsm response when incoming call is indicated
                readDTMFCommand();        
        }
}

void sendATCommand(String atCommand) {          
        gsm.println(atCommand);
}

String receiveGSMResponse() {
        boolean isAvailable = false;
        //TODO check diff between String message = "" to message = String("")
        String gsmResponseMessage = "";
        
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
        
        return gsmResponseMessage;
}

void deleteAllSMS() {
        //TODO use option <delflag> to delete all messages from sim
        //ex: AT+CMGD=1,4
        String atCommand = "AT+CMGD=1,4";
        sendATCommand(atCommand);
        delay(1000);
}

void readSMSCommand(String gsmResponseMessage) {
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
                switchOnStarterRelay(false);
                sendSMSAlert("\n\nEngine STOP.\n\n");
                sendLocation();          
        }
        if(command == "OVERRIDE" && password == userPassword) {
                switchOnStarterRelay(true);
                sendSMSAlert("\n\nIgnition is ON.\n\n");                  
        }
        if(command == "RENEW" && password == userPassword) {          
                activeDeleteUserFingerPrint = true;                  
        }        
}

void sendSMSAlert(String message) {
        //TODO test delay time of sending sms
        Serial.println("Sending SMS...");
        //TODO is this the same with "\37"
        char controlZ = 0x1A;
        String atCommand = "AT+CMGS= \"";
        
        atCommand += userNumber + "\"";
        sendATCommand(atCommand);
        delay(90);
        
        atCommand = message + controlZ;
        sendATCommand(atCommand);
        //TODO study error handling when message was not sent.
        Serial.println("Message sent!");
        delay(90);
}

void switchOnStarterRelay(boolean mode) {
        if(mode) {
                //TODO make messages as a field.
                Serial.println("\n\nIgnition is ON.\n\n");
                starterRelayIsOff = false;
                digitalWrite(starterRelay, HIGH);        
        } else {
                Serial.println("\n\nEngine STOP.\n\n");
                starterRelayIsOff = true;
                digitalWrite(starterRelay, LOW);         
        }
}

void checkFingerPrint() {
        receiveResponsePacket();  
        if((fpShieldResponsePacket[9] == 1)) { 
                Serial.println("Finger print PASSED.\n\n");
                switchOnStarterRelay(true);            
        } else {
                Serial.println("Finger print FAILED. Please try again.\n\n");      
        }        
        clearPacket(fpShieldResponsePacket);            
}
  
void clearPacket(byte *packet) {
  	for(int i=0; i<=50; i++) {
  		packet[i] = 0x00;	
  	}
}

void scanFingerPrint() {
        Serial.print("Scanning finger print");  
	clearPacket(fpShieldCommandPacket);
	fpShieldCommandPacket[0] = 0x55;
 	fpShieldCommandPacket[1] = 0xAA;
  	fpShieldCommandPacket[2] = 0x02;
  	fpShieldCommandPacket[3] = 0x01;

	sendCommandPacket();
        delay(500);       
        Serial.println(".");       
}

void sendCommandPacket() {
	getCheckSum();
	Serial.write(fpShieldCommandPacket,24);
        Serial.print(".");
}

void getCheckSum() {
	int checkSum = 0;
	
	for(int i=0; i<=21;i++) {
		checkSum += fpShieldCommandPacket[i];
	}
	fpShieldCommandPacket[22] = checkSum & 0xFF;
	fpShieldCommandPacket[23] = (checkSum - (checkSum & 0xFF))/256;
        Serial.print(".");
}

void receiveResponsePacket() {
	int i = 0;
        Serial.println("\n\nReceiving response packet");
	while(Serial.available()>0) {
		fpShieldResponsePacket[i] = Serial.read();
                Serial.print(i);
                Serial.print("-----");
                Serial.println(fpShieldResponsePacket[i]);
		i++;
	}
        Serial.println("Complete\n\n");
}

void deleteAllFingerPrint() {
        Serial.print("Deleting all user finger print");
        clearPacket(fpShieldCommandPacket);
        fpShieldCommandPacket[0] = 0x55;
        fpShieldCommandPacket[1] = 0xAA;
        fpShieldCommandPacket[2] = 0x06;  
        fpShieldCommandPacket[3] = 0x01;
        
        sendCommandPacket();
        delay(500);
        Serial.println(".");        
        Serial.println("Done\n\n");        
}

void addUserFingerPrint() {
        Serial.print("Add user finger print");
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
        Serial.println(".");         
}

char getDTMF() {
        Serial.print("DTMF: ");
        String dmtfCommand = "";
        char thisChar = '\0';
        dtmf.sample(dtmfSensor);
        dtmf.detect(dMags, 506);
        thisChar = dtmf.button(dMags, 1800.);
        if(thisChar) {
                Serial.print(thisChar);                
        }
        
        return thisChar;      
}

void readDTMFCommand() {
        int index = 0;
        String dtmfCode = "";
        
        while(index < userPassword.length()) {
            dtmfCode += getDTMF();
            index++;            
        }
        
        if(dtmfCode == userPassword) {
            switchOnStarterRelay(false);
        }
}

String locationMessage;
void getDirection() {
        float motorSpeed = Serial.read(speedMeter);
        float leftBearing = Serial.read(leftSwitch);
        float rightBearing = Serial.read(rightSwith);
        
        if(motorSpeed) {
            locationMessage += "Speed: "+motorSpeed;
            
            if(leftBearing) {
                locationMessage += "Left: "+leftBearing;
            } else if(rightBearing) {
                locationMessage += "Right: "+rightBearing;
            } else {
                locationMessage += "Straight ahead";
            }
            //TODO add time lapse when changing directions or speed
            //use AT+CLTS to get time stamp.
            locationMessage += "\n";
        }
}

void sendLocation() {
        int i;
        for(i = 150; i < locationMessage.length(); i+=150) {
              sendSMSAlert(locationMessage.substringOf(i));          
        }
        
        sendSMSAlert(locationMessage.substringOf(i));
}
