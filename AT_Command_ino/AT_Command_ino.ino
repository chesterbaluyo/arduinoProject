#include <SoftwareSerial.h>

SoftwareSerial gsm(0,1);

String gsmResponseMessage = "";
String userNumber = "09999969515";
String userPassword = "123456";
int smsCount = 0;
int starterRelay = 8;

void setup() {
          gsm.begin(9600);
          pinMode(8, OUTPUT);
          digitalWrite(8, LOW);
                   
          delay(2000);
          initializeGSM();        
}

void loop() {
          if(gsm.available()) {
                  receiveGSMResponse();
                  if(gsmResponseMessage.substring(2,7)=="+CMT:"){
                          readSMSCommand();                        
                          smsCount++;
                          if(smsCount == 36) {
                                  deleteAllSMS();
                          }
                  }
                  clearGsmResponseMessage();                  
          }
}

void initializeGSM() {
          sendATCommand("AT");
          delay(500);
          sendATCommand("AT+CMGF=1");
          delay(500);
          deleteAllSMS();          
}

void sendATCommand(String atCommand) {          
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
          gsmResponseMessage = String("");
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
