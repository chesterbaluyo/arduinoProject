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
          for(int i=1;i<=40;i++) {
                  String atCommand = "AT+CMGD=";
                  atCommand += i;
                  sendATCommand(atCommand);
                  delay(500);
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
          }
          if(command == "CHANGE_ID" && password == userPassword) {
            
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
