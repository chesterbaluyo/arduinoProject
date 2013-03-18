#include <SoftwareSerial.h>

SoftwareSerial gsm(2,3);
String gsmResponseMessage = "";

void setup() {
          gsm.begin(9600);
          Serial.begin(19200);
          Serial.println("Start GSM");
          Serial.println("");
          Serial.println("");          
          delay(2000);
          initializeGSM();        
}

void loop() {
          if(gsm.available()) {
                  receiveGSMResponse();
                  if(gsmResponseMessage.substring(2,7)=="+CMT:"){
                          Serial.print("Sender Number: ");
                          Serial.println(gsmResponseMessage.substring(9,22));                  
                  }
                  clearGsmResponseMessage();                  
          }
}

void initializeGSM() {
          sendATCommand("AT");
          delay(500);
          sendATCommand("AT+CMGF=1");
          delay(500);
          readAllSMS();
          deleteAllSMS();          
          Serial.println("Success!");
}

void sendATCommand(String atCommand) {
          Serial.print(atCommand);
          delay(2000);
          Serial.println(" is sending...");          
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
          message = String("");
          delay(5000);
}

void deleteAllSMS() {
                  String atCommand = "AT+CMGD=1";
                  sendATCommand(atCommand);
                  delay(500);
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
