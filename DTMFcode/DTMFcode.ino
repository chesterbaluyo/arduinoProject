#include <SoftwareSerial.h>

int keyvalue;  // the number associated with the tone

void setup()
{
 Serial.begin(9600);
 pinMode(6, INPUT); //Binary 1
 pinMode(5, INPUT); //Binary 2
 pinMode(4, INPUT); //Binary 3
 pinMode(3, INPUT); //Binary 4
 pinMode(2, INPUT); //Steering (StD)
}

void loop()
{ if (digitalRead(2) == HIGH) {
  
 if (digitalRead(6) == HIGH) { keyvalue = 1; } 
   else { keyvalue = 0; } 
 if (digitalRead(5) == HIGH) { keyvalue = keyvalue + 2; }
 if (digitalRead(4) == HIGH) { keyvalue = keyvalue + 4; } 
 if (digitalRead(3) == HIGH) { keyvalue = keyvalue + 8; }

 // 'Special' char tones
 if (keyvalue == 10) { Serial.println("0"); }
   else { if (keyvalue == 11) { Serial.println("*");  }
   else { if (keyvalue == 12) { Serial.println("#");  }
 
 // Anything else, print
   else { Serial.println(keyvalue);
          }
   }
  }
 }
 delay(100);
}
