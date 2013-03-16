#include <SoftwareSerial.h>

SoftwareSerial mySerial(2,3)
byte fpShieldCommandPacket[24]
byte fpShieldResponsePacket[48]

String gsmMessage = String("");

void setup() {

	Serial.begin(38400);
//	initializeGSM();

	mySerial.begin(115200);
	initializeFingerPrint();
			
}

void loop() {
//	readGsmSMS();
}

void initializeFingerPrint() {
	
	readFingerPrint();
	matchFingerPrint();
}

void clearPacket(byte *packet) {
	for(int i=0; i<=50; i++) {
		packet[i] = 0x00;	
	}
}

void readFingerPrint() {

	Serial.println("Enter Fingerprint");

	clearPacket(fpShieldCommandPacket);
	fpShieldCommandPacket[0] = 0x55;
 	fpShieldCommandPacket[1] = 0xAA;
  	fpShieldCommandPacket[2] = 0x02;
  	fpShieldCommandPacket[3] = 0x01;

	sendCommandPacket();
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

int receiveResponsePacket() {
	return mySerial.read();
}

void matchFingerPrint() {
	int i = 0;

	while(mySerial.available) {
		fpShieldResponsePacket[i] = receiveResponsePacket();
		i++;
	}

	if(fpResponsePacket[30] == 20) {
		Serial.println("Finger Match..");
		clearPacket(fpShieldResponsePacket);
	}
}

