byte ledpins[] = { 8,9,10,11,13 };
byte ledstatus[] = { 0,0,0,0,0 };
byte ledcount = 5;

#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3); // RX, TX

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  Serial.println("Starting ...");
  mySerial.println("Starting ...");
  for (int i=0; i<ledcount; i++) {
    pinMode(ledpins[i],OUTPUT);
    digitalWrite(ledpins[i],LOW);
  }
}

void resetleds() {
  Serial.println("Resetting all leds");
  mySerial.println("Resetting all leds");
  for (int i=0; i<ledcount; i++) {
    ledstatus[i]=0;
    digitalWrite(ledpins[i],LOW);
  }
}

void lightall() {
  Serial.println("All leds on");
  mySerial.println("All leds on");
  for (int i=0; i<ledcount; i++) {
    ledstatus[i]=1;
    digitalWrite(ledpins[i],HIGH);
  }
}
void loop() {
  char c = mySerial.read();
  if (c=='n' || c=='N') resetleds();
  if (c=='a' || c=='A') lightall();
  int led = c-48;
  if (led >=0  && led < ledcount) {
    Serial.print("Flipping led ");
    Serial.println(led);
    mySerial.print("Flipping led ");
    mySerial.println(led);
    ledstatus[led]=1-ledstatus[led];
    digitalWrite(ledpins[led],ledstatus[led]);
  }
}
