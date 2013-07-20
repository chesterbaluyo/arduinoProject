#include <SoftwareSerial.h>
#include <DTMF.h>

/* FingerPrint Shield */
SoftwareSerial fingerPrint(10, 11);
byte fpShieldCommandPacket[24];
byte fpShieldResponsePacket[48];

/* GSM Shield */
SoftwareSerial gsm(2,3);
//TODO get userNumber on sim phonebook
String userNumber = "09352983630";
String userPassword = "123456";
String locationLog = "";

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
int speedMeter = A1;
int potentiometer = A2;

void setup() {
        gsm.begin(9600);
        fingerPrint.begin(115200);
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
        clearPacket(fpShieldResponsePacket, 48);     
}

void loop() {
        if(digitalRead(ignitionSwitch) == LOW) {        
                if(starterRelayIsOff) {
                        scanFingerPrint();
                        setPacketResponseTimeOut(5000);         
                }
        }
        
        gsmCallAndSMSListener();

        //TODO make sendNotification();
}

void initializePin() {
        Serial.print("Initialize Sensors and Switches: ");
        pinMode(starterRelay, OUTPUT);
        pinMode(ignitionSwitch, INPUT);
        
        digitalWrite(starterRelay, LOW);  
        Serial.println("Done");        
}

void initializeGSM() {
        Serial.println("Initialize GSM: ");
        sendATCommand("AT");
        waitForAndGetGSMResponse(1000);                  
}

void setSMSMessageFormat() {
        Serial.println("Set SMS message format: ");  
        sendATCommand("AT+CMGF=1");
        waitForAndGetGSMResponse(1000);
}

void enableAutomaticAnswer() {
        Serial.println("Enable automatic answering: ");  
        sendATCommand("ATS0=1");
        waitForAndGetGSMResponse(1000);
}

void enableLoudSpeaker() {
        Serial.println("Loud speaker mode: ");   
        sendATCommand("ATM9"); 
        waitForAndGetGSMResponse(1000);
}

void setLoudnessToMax() {
        Serial.println("Set maximum loudness: ");   
        sendATCommand("ATL9");
        waitForAndGetGSMResponse(1000);          
}

void gsmCallAndSMSListener() {  
        String gsmResponseMessage = getGSMResponse();
        
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

String waitForAndGetGSMResponse(int timeOut) {
        String gsmResponse = "";
        
        for(int timeCount = 1, timeDelay = 100; timeCount*timeDelay <= timeOut; timeCount++) {
            gsmResponse = getGSMResponse();
            
            if(gsmResponse.endsWith("OK\r\n")) {
                    break;      
            }
            
            delay(timeDelay);
        }
        
        return gsmResponse;
}

String getGSMResponse() {
        boolean isAvailable = false;
        String gsmResponseMessage = "";
        gsm.listen();
        
        while(gsm.available()>0) {
                char incomingData = '\0';
                incomingData = gsm.read(); 
                gsmResponseMessage += incomingData;
                isAvailable = true;
        }
 
        if(isAvailable) {
                Serial.println(gsmResponseMessage);          
        }
        
        return gsmResponseMessage;
}

void deleteAllSMS() {
        Serial.println("Delete All SMS: ");
        sendATCommand("AT+CMGD=1,4");
        waitForAndGetGSMResponse(5000);
}

void readSMSCommand(String gsmResponseMessage) {
        String command;
        String message;
        
        message = parseMessage(gsmResponseMessage);
        command = message;
        command.toUpperCase();
        
        if(command.startsWith("STOP") && message.endsWith(userPassword)) {
                switchOnStarterRelay(false);
                sendSMS("\n\nEngine STOP.\n\n");
                //sendLocation(locationLog);          
        }
        if(command.startsWith("OVERRIDE") && message.endsWith(userPassword)) {
                switchOnStarterRelay(true);
                sendSMS("\n\nIgnition is ON.\n\n");                  
        }
        if(command.startsWith("RENEW") && message.endsWith(userPassword)) {
                if(starterRelayIsOff) {
                        deleteAllFingerPrint();                
                        delay(2000);
                        addUserFingerPrint();
                        setPacketResponseTimeOut(10000);                
                }          
        }        
}

String parseMessage(String sms) {
        int messageLocation = 0;
        
        while(sms.indexOf("\"")>=0) {
                messageLocation = sms.indexOf("\"");
                sms = sms.substring(messageLocation + 1);
        }
        
        return sms.substring(2);
}

/*
 * Sends SMS message
 *
 * @REQUIRED message
 *
 * When unable to send SMS the GSM device waits at most 16s to respond. 
 **/
void sendSMS(String message) {
        Serial.println("Sending SMS: ");
        char controlZ = 0x1A;
        String atCommand = "AT+CMGS=\"";
        
        atCommand += userNumber + "\"";
        sendATCommand(atCommand);
        waitForAndGetGSMResponse(1000);
        
        atCommand = message + controlZ;
        sendATCommand(atCommand);
        waitForAndGetGSMResponse(15000);
}

String getTime() {
            sendATCommand("AT+CCLK?");
            String time = "Time: " + waitForAndGetGSMResponse(1000).substring(29,37);
            
            return time;   
}

void switchOnStarterRelay(boolean mode) {
        if(mode) {
                //TODO make messages as a field.
                Serial.println("\n\nIgnition is ON.\n\n");               
                starterRelayIsOff = false;
                digitalWrite(starterRelay, HIGH);  
                //TODO setTime() to 00:00:00  
        } else {
                Serial.println("\n\nEngine STOP.\n\n");
                starterRelayIsOff = true;
                digitalWrite(starterRelay, LOW);         
        }
}

void checkFingerPrint() {  
        if((fpShieldResponsePacket[9] == 2)) { 
                Serial.println("Finger print PASSED.\n\n");
                switchOnStarterRelay(true);            
        } else {
                if(getCheckSum(fpShieldResponsePacket, 48)) {
                        Serial.println("Finger print FAILED.\n\n");                      
                } else {
                        Serial.println("*** Finger Print Shield not available! ***");
                        fpCancel();
                }
        }
        
        clearPacket(fpShieldResponsePacket, 48);            
}
  
void clearPacket(byte *packet, int packetSize) {
  	for(int index = 0; index < packetSize; index++) {        
  		packet[index] = 0x00;	
  	}
}

void scanFingerPrint() {
        Serial.println("Scanning finger print: ");  
	clearPacket(fpShieldCommandPacket, 24);
	fpShieldCommandPacket[0] = 0x55;
 	fpShieldCommandPacket[1] = 0xAA;
  	fpShieldCommandPacket[2] = 0x02;
  	fpShieldCommandPacket[3] = 0x01;

	sendCommandPacket();       
}

void sendCommandPacket() {
	int checkSum = getCheckSum(fpShieldCommandPacket, 22);
	fpShieldCommandPacket[22] = checkSum & 0xFF;
	fpShieldCommandPacket[23] = (checkSum - (checkSum & 0xFF))/256;
        
	fingerPrint.write(fpShieldCommandPacket,24);
        delay(500);
}

int getCheckSum(byte *packet, int packetSize) {
	int checkSum = 0;
	
	for(int i = 0; i < packetSize; i++) {
		checkSum += packet[i];
	}

        return checkSum;
}

void getResponsePacket() {
	int index = 0;
        fingerPrint.listen();
        
	while(fingerPrint.available() > 0) {
		fpShieldResponsePacket[index] = fingerPrint.read();
                //Serial.print(i);
                //Serial.print("-----");
                //Serial.println(fpShieldResponsePacket[i]);
		index++;
	}
}

void deleteAllFingerPrint() {
        Serial.print("Deleting all user finger print: ");
        clearPacket(fpShieldCommandPacket, 24);
        fpShieldCommandPacket[0] = 0x55;
        fpShieldCommandPacket[1] = 0xAA;
        fpShieldCommandPacket[2] = 0x06;  
        fpShieldCommandPacket[3] = 0x01;
        
        sendCommandPacket();
        Serial.println("Done");
}

void addUserFingerPrint() {
        Serial.println("Add user finger print: ");
        clearPacket(fpShieldCommandPacket, 24);
        fpShieldCommandPacket[0] = 0x55;
        fpShieldCommandPacket[1] = 0xAA;
        fpShieldCommandPacket[2] = 0x03;
        fpShieldCommandPacket[3] = 0x01;
        fpShieldCommandPacket[4] = 0x02;
        fpShieldCommandPacket[5] = 0x00;
        fpShieldCommandPacket[6] = 0x00;
        fpShieldCommandPacket[7] = 0x01;  
        
        sendCommandPacket();
}

void fpCancel() {
        clearPacket(fpShieldCommandPacket, 24);
        fpShieldCommandPacket[0] = 0x55;
        fpShieldCommandPacket[1] = 0xAA;
        fpShieldCommandPacket[2] = 0x30;
        fpShieldCommandPacket[3] = 0x01; 
        
        sendCommandPacket();
}

void setPacketResponseTimeOut(int timeOut) {
        for(int timeCount = 1, timeDelay = 100; timeCount*timeDelay <= timeOut; timeCount++) {
            getResponsePacket();
            delay(timeDelay);
        }
        
        checkFingerPrint();
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
        String dtmfCode = "";
        
        while(dtmfCode.length() < userPassword.length()) {
                dtmfCode += getDTMF(); 
        }
        
        Serial.print("\n\nVerify: ");
        Serial.println(dtmfCode);
        if(dtmfCode == userPassword) {
                switchOnStarterRelay(false);
        }
}

//TODO make precise directions
String getDirection() {
        String directions = "";
        int motorSpeed = analogRead(speedMeter);
        int leftBearing = analogRead(leftSwitch);
        int rightBearing = analogRead(rightSwitch);
        
        if(motorSpeed) {
                directions += "\nSpeed: "+ motorSpeed;
                directions += getTime();
        }
        
        if(leftBearing) {
                directions += "\nLeft: " + leftBearing;
                directions += getTime();
        } 
        
        if(rightBearing) {
                directions += "\nRight: " + rightBearing;
                directions += getTime();
        }
        
        return directions;
}

void sendLocation(String location) {
        int SMS_MAX_LENGTH = 150;
        int index = 0;
        
        if(location.length() > SMS_MAX_LENGTH) {
                while(location.length() > index) {
                        Serial.println(location.substring(index, index + SMS_MAX_LENGTH));
                        sendSMS(location.substring(index, index + SMS_MAX_LENGTH)); 
                        index += (SMS_MAX_LENGTH + 1);         
                }
                 
                sendSMS(location.substring(index));
        } else {
                if(location.length()) {
                        sendSMS(location);
                }
        }
}
