#include <SoftwareSerial.h>
#include <DTMF.h>

/* FingerPrint Shield */
SoftwareSerial fingerPrint(10, 11);
byte fpShieldCommandPacket[24];
byte fpShieldResponsePacket[48];

/* GSM Shield */
SoftwareSerial gsm(2,3);
String userNumber;
String password;
String locationLog = "";

/* DTMF Decoder */
float n = 152.0;
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
        setLoudSpeakerVolumeLevel();
        delay(500);
        deleteAllSMS();
        delay(500);       
        //Add default user name and password        
        initializeUserNumberAndPassword("09996219071", "0987654321"); 
        delay(500);
        clearPacket(fpShieldResponsePacket, 48);     
}

void loop() {
        if(digitalRead(ignitionSwitch) == LOW) {        
                if(starterRelayIsOff) {
                        scanFingerPrint();
                        waitForAndCheckPacketResponse(5000);
                        //To test DTMF command commented out the code below.
                        //localDTMF(); 
                } else {
                        switchOnStarterRelay(false);
                }  
        }
        
        gsmCallAndSMSListener();
        getLocation();
        sendNotification();        
}

void localDTMF() {
        //AT+VTS=
        Serial.println("Local DTMF: ");
        sendATCommand("AT+CLDTMF=5,\"0,0,9,3,8,4,7,7,9,6,2,5,4,1,3,3,2,0,1,1\"");
        Serial.print(waitForAndGetGSMResponse(1000));
        readDTMFCommand();
}

void initializePin() {
        Serial.println("Initialize Sensors and Switches: ");
        pinMode(starterRelay, OUTPUT);
        pinMode(ignitionSwitch, INPUT);
        
        digitalWrite(starterRelay, LOW);  
        Serial.println("Done");        
}

void initializeGSM() {
        Serial.println("Initialize GSM: ");
        sendATCommand("AT");
        Serial.print(waitForAndGetGSMResponse(1000));                  
}

void setSMSMessageFormat() {
        Serial.println("Set SMS message format: ");  
        sendATCommand("AT+CMGF=1");
        Serial.print(waitForAndGetGSMResponse(1000));
}

void enableAutomaticAnswer() {
        Serial.println("Enable automatic answering: ");  
        sendATCommand("ATS0=1");
        Serial.print(waitForAndGetGSMResponse(1000));
}

void enableLoudSpeaker() {
        Serial.println("Loud speaker mode: ");   
        sendATCommand("ATM9"); 
        Serial.print(waitForAndGetGSMResponse(1000));
}

void setLoudnessToMax() {
        Serial.println("Set maximum loudness: ");   
        sendATCommand("ATL9");
        Serial.print(waitForAndGetGSMResponse(1000));
}

void setLoudSpeakerVolumeLevel() {
        Serial.println("Loudspeaker volume level: ");   
        sendATCommand("AT+CLVL=100");
        Serial.print(waitForAndGetGSMResponse(1000));
}

void initializeUserNumberAndPassword(String defaultUserNumber, String defaultPassword) {
        String result = findPBookEntry("user"); 
        if(result.indexOf(": ") < 0) {
                userNumber = defaultUserNumber;
        } else {
                userNumber = getNumber(result);                                                   
        }
        
        result = findPBookEntry("password");
        if(result.indexOf(": ") < 0) {
                password = defaultPassword;
        } else {
                password = getNumber(result);                          
        }        
        
        Serial.println("Sending number: ");
        Serial.println(userNumber);
        Serial.println("Password: ");
        Serial.println(password);
}

void gsmCallAndSMSListener() {  
        String gsmResponseMessage = getGSMResponse();
        
        if(gsmResponseMessage.length() > 0) {        
                Serial.println(gsmResponseMessage);
                
                if(gsmResponseMessage.startsWith("+CMT",2)){
                        readSMSCommand(gsmResponseMessage);                        
                        deleteAllSMS();                   
                }
                else if (gsmResponseMessage.startsWith("+CTI",2) || gsmResponseMessage.startsWith("+CRING",2)) {
                        //+CRING
                        //TODO check if correct gsm response when incoming call is indicated
                        readDTMFCommand();        
                }
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
            } else if(gsmResponse.endsWith("ERROR\r\n")) {
                    break;
            }
            
            delay(timeDelay);
        }
        
        return gsmResponse;
}

String getGSMResponse() {
        String gsmResponseMessage = "";
        gsm.listen();
        
        while(gsm.available()>0) {
                char incomingData = '\0';
                incomingData = gsm.read(); 
                gsmResponseMessage += incomingData;
        }

        return gsmResponseMessage;
}

void deleteAllSMS() {
        Serial.println("Delete all Inbox: ");        
        sendATCommand("AT+CMGDA=\"DEL INBOX\"");
        Serial.print(waitForAndGetGSMResponse(5000));
}

void readSMSCommand(String gsmResponseMessage) {
        String command;
        String message;
        
        message = parseMessage(gsmResponseMessage);
        command = message;
        command.toUpperCase();
        
        if(command.startsWith("STOP") && message.endsWith(password + "\r\n")) {
                switchOnStarterRelay(false);
                sendSMS("Engine STOP.", true);
                //TODO send stored messages.
                //use AT+CMSS=index. Delete all messages after sending. 
        }
        if(command.startsWith("OVERRIDE") && message.endsWith(password + "\r\n")) {
                switchOnStarterRelay(true);
                sendSMS("Override: " + gsmResponseMessage.substring(5), true);                  
        }
        if(command.startsWith("RENEW") && message.endsWith(password + "\r\n")) {
                if(starterRelayIsOff) {
                        deleteAllFingerPrint();                
                        delay(2000);
                        addUserFingerPrint();
                        waitForAndCheckPacketResponse(10000);                
                }          
        }        
}

String parseMessage(String sms) {
        int messageLocation = 0;
        messageLocation = sms.lastIndexOf("\"") + 3;

        return sms.substring(messageLocation);
}

/**
 * Sends SMS message
 *
 * @REQUIRED message
 * mode
 * Set mode to true then it will be sent to the user else it will be saved to sim memory.
 * When unable to send SMS the GSM device waits at most 16s to respond. 
 */
void sendSMS(String message, boolean mode) {
        Serial.print("Sending SMS \"");
        Serial.print(message);
        Serial.println("\": ");
        String atCommand = "";
        
        char controlZ = 0x1A;
        if(mode) {
                atCommand = "AT+CMGS=\"";        
        } else {
                atCommand = "AT+CMGW=\"";                  
        }
        
        atCommand += userNumber + "\"";
        sendATCommand(atCommand);
        Serial.println(waitForAndGetGSMResponse(2000));
        
        atCommand = message + controlZ;
        sendATCommand(atCommand);
}

String findPBookEntry(String query) {
        String result = "";
        String search = "AT+CPBF=\"";
        
        search += query + "\"";
        sendATCommand(search);        
        result = waitForAndGetGSMResponse(2000);
        
        return result;
}

String getNumber(String entry) {
        int startQuote = entry.indexOf(": ");
        int endQuote = 0;
        
        startQuote = entry.indexOf("\"", startQuote) + 1;
        endQuote = entry.indexOf("\"", startQuote); 
        
        return entry.substring(startQuote , endQuote);        
}

String getTime() {
        sendATCommand("AT+CCLK?");
        String time = waitForAndGetGSMResponse(1000).substring(29,37);
        
        return time;   
}

void resetTime() {
        Serial.println("Reset time: ");
        sendATCommand("AT+CCLK=\"13/07/28,00:00:00+08\"");
        Serial.print(waitForAndGetGSMResponse(1000));
}

void switchOnStarterRelay(boolean mode) {
        if(mode) {
                Serial.println("\n------------------------");
                Serial.println("Ignition is ON.\n");               
                starterRelayIsOff = false;
                digitalWrite(starterRelay, HIGH);  
                locationLog = ""; 
                resetTime();  
        } else {
                Serial.println("\n------------------------");          
                Serial.println("Engine STOP.\n");
                starterRelayIsOff = true;
                digitalWrite(starterRelay, LOW);         
        }
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

void waitForAndCheckPacketResponse(int timeOut) {
        for(int timeCount = 1, timeDelay = 100; timeCount*timeDelay <= timeOut; timeCount++) {
            getResponsePacket();
            
            if((fpShieldResponsePacket[9] == 2)) { 
                    Serial.println("Finger print PASSED.\n\n");
                    switchOnStarterRelay(true);
                    break;            
            }            
            
            delay(timeDelay);
        }
        
        if((fpShieldResponsePacket[9] != 2)) {
                if(getCheckSum(fpShieldResponsePacket, 48)) {
                        Serial.println("Finger print FAILED.\n\n");                      
                } else {
                        Serial.println("*** Finger Print Shield not available! ***");
                        fpCancel();
                }        
        }
        
        clearPacket(fpShieldResponsePacket, 48);        
}

char getDTMF() {
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
        Serial.println("DTMF: ");
        String dtmfCode = "";
        
        for(int index = 0; password.length() > index; index++) {
                unsigned long startTime = millis(), currentTime = 0;
                int timeLapse = 0, TIME = 3000;
                 
                while(TIME >= timeLapse) {
                        char dtmf = getDTMF();
                        if(dtmf == password.charAt(index)) {
                            dtmfCode += dtmf;
                            Serial.println(dtmfCode);
                            break;
                        }
                        currentTime = millis();
                        timeLapse = currentTime - startTime;
                }
        } 
      
        if(dtmfCode.equals(password)) {
                Serial.println("DTMF PASSED!");          
                sendConfirmation();
                switchOnStarterRelay(false);
        } else {
                Serial.println("Try again.");
        }
}

void sendConfirmation() {
        sendATCommand("AT+CLDTMF=20,\"*,*,*\"");
        waitForAndGetGSMResponse(1000);
        sendSMS("Engine STOP!", true);
}

String getDirection() {
        const int RESISTOR_MAX_ANGLE = 270;
        const int RESISTOR_MAX_VALUE = 1050;
        const int STEP = 10;
        float resistorValue = analogRead(potentiometer);
        String directions = "";
        static int lastShift = 0;  
        int shift = RESISTOR_MAX_ANGLE / 2 - resistorValue / RESISTOR_MAX_VALUE * RESISTOR_MAX_ANGLE;
        
        if(shift >= (lastShift + STEP)) {
              lastShift = shift;
              directions = readDirection(shift) + "\n";
        }       
        else if(shift <= (lastShift - STEP)) {
              lastShift = shift;        
              directions = readDirection(shift) + "\n";
        }
        
        return directions;
}

String readDirection(int angle) {
        String directions = "";
        
        if(angle > 0) {
                directions = String(angle) + "L";                  
        } else {
                directions = String(-angle) + "R";  
        }
        directions += getTime();
        
        return directions;
}

int getDistance() {
        const int OFFSET = 5;
        static unsigned long startTime = millis();
        unsigned long time = 0;
        static int initialSpeed = 0;          
        float currentSpeed = analogRead(speedMeter);
        int distance = 0;
        
        if(currentSpeed >= (initialSpeed + OFFSET)) {
              time = millis();
              time -= startTime;          
              //TODO convertion
              distance += initialSpeed * time;
              
              initialSpeed = currentSpeed;
              startTime = time;
        }
        else if(currentSpeed <= (initialSpeed - OFFSET)) {
              time = millis();
              time -= startTime;          
              //TODO convertion
              distance += initialSpeed * time;
              
              initialSpeed = currentSpeed;
              startTime = time;              
        }
              
        return distance;      
}

int totalDistance = 0;
void getLocation() {
        int distance = getDistance();
        if(distance) {
                totalDistance += distance;
                locationLog += totalDistance; 
                locationLog += getDirection();
        }
}

void sendNotification() {
        //TODO add also when bike has been picked up or device has been disconnected.
        if(starterRelayIsOff) {
                String hasChange = getDirection();
                if(hasChange.length()) {                  
                        sendSMS("ALERT! " + hasChange, true);
                }
                
                int distanceChange = getDistance();
                if(distanceChange) {                  
                        sendSMS("ALERT! " + distanceChange, true);
                }                
        } else {
                if(locationLog.length() > 140) {
                        //Save locationLog to message storage.
                        //use AT+CMGW=index and check if message is stored when sending fails.
                        locationLog = "";
                }
        }
}
