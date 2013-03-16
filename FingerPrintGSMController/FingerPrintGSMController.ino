#include <SoftwareSerial.h>

SoftwareSerial mySerial(2,3);
byte fpShieldCommandPacket[24];
byte fpShieldResponsePacket[48];

String gsmMessage = String("");

void setup() {

	Serial.begin(38400);
//	initializeGSM();

	mySerial.begin(115200);
	initializeFingerPrint();
			
}

void loop() {
//	readGsmSMS();
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
        
        if(mySerial.available()) {
                receiveResponsePacket();
                
               	if(fpShieldResponsePacket[30] == 20 || fpShieldResponsePacket[8] == 20) {
		        Serial.println("Finger Match..");
	        }

	        clearPacket(fpShieldResponsePacket);
        }
}

void initializeFingerPrint() {             
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
	mySerial.write(fpShieldCommandPacket,24);
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
	while(mySerial.available()) {
		fpShieldResponsePacket[i] = mySerial.read();
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
