#include <SoftwareSerial.h>


SoftwareSerial gsm(0,1);
SoftwareSerial fingerPrint(2,3);

byte fpShieldCommandPacket[24];
byte fpShieldResponsePacket[48];
String gsmResponseMessage = "";
String userNumber = "09999969515";
String userPassword = "123456";
int smsCount = 0;

int starterRelay = 6;
int fpSwitch = 7;
int speedMeter = 8;
int leftSwitch = 9;
int rightSwitch = 10;

void setup() {

	Serial.begin(38400);
        gsm.begin(9600);
	fingerPrint.begin(115200);

        initializePin();
        initializeGSM();			
}

void loop() {

        if(Serial.available()) {
                char option;
            
                Serial.println("Select Option:");
                Serial.println("[1]Delete All");
                Serial.println("[2]Enroll FingerPrint");
                Serial.println("[3]Read Finger Print");
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
        
        if(fingerPrint.available()) {
                receiveResponsePacket();
                
               	if(fpShieldResponsePacket[30] == 20 || fpShieldResponsePacket[8] == 20) {
		        Serial.println("Finger Match..");
	        }

	        clearPacket(fpShieldResponsePacket);
        }
}

void initializePin() {
        pinMode(starterRelay, OUTPUT);
        pinMode(fpScanSwitch, INPUT);
        pinMode(speedMeter, INPUT);
        pinMode(leftSwitch, INPUT);
        pinMode(rightSwitch, INPUT);
        
        digitalWrite(starterRelay, LOW);  
}

void clearPacket(byte *packet) {
	for(int i=0; i<=50; i++) {
		packet[i] = 0x00;	
	}
}

void readFingerPrint() {
	clearPacket(fpShieldCommandPacket);
	fpShieldCommandPacket[0] = 0x55;
 	fpShieldCommandPacket[1] = 0xAA;
  	fpShieldCommandPacket[2] = 0x02;
  	fpShieldCommandPacket[3] = 0x01;

	sendCommandPacket();
        delay(500);
	Serial.println("Enter Fingerprint");        
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
                Serial.print(i);
                Serial.print("----");
                Serial.println(fpShieldResponsePacket[i]);
		i++;
	}
	int checkSum = 0;
	
	for(int j=0; j<=21;j++) {
		checkSum += fpShieldResponsePacket[j];
	}

        Serial.print("+++++++++++++");
        Serial.print(checkSum&0xFF);
        Serial.print("--");
        Serial.println((checkSum - (checkSum & 0xFF))/256);
        
	if(fpShieldResponsePacket[30] == 20 || fpShieldResponsePacket[8] == 20) {
		Serial.println("Finger Match..");
	}
	clearPacket(fpShieldResponsePacket);
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
