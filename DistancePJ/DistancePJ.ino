#include <LiquidCrystal.h>

#define trigPin 13 // define TrigPin
#define echoPin 12 // define EchoPin.
#define pinBuzzer 7

#define MAX_DISTANCE 200 // Maximum sensor distance is rated at 400-500cm.
// define the timeOut according to the maximum range. timeOut= 2*MAX_DISTANCE /100 /340 *1000000 = MAX_DISTANCE*58.8
float timeOut = MAX_DISTANCE * 60; 
int soundVelocity = 340; // define sound speed=340m/s
LiquidCrystal lcd(11, 10, 5, 4, 3, 2);
int buzzerDelay = 20;


void setup() {
  pinMode(trigPin,OUTPUT);// set trigPin to output mode
  pinMode(echoPin,INPUT); // set echoPin to input mode
  pinMode(pinBuzzer,OUTPUT);
  
  Serial.begin(9600);    // Open serial monitor at 9600 baud to see ping results.
  lcd.begin(16, 2);
  lcd.noAutoscroll();
  lcd.print("MISURO DISTANZE");
  lcd.setCursor(0,1);
  lcd.print("PLEASE WAIT!!!");
  delay(2000);
  lcd.clear();
}

void loop() {
  delay(100); // Wait 100ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
  float distance = getSonar();
  /*
  Serial.print("Ping: ");
  Serial.print(distance); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.println("cm"); 
  */
  lcd.setCursor(0,0);
  lcd.print("Dist: ");
  lcd.print(distance);
  lcd.print("cm"); 
  if(distance < 50) {
    lcd.setCursor(0,1);
    lcd.print("TOO CLOSE, MAN");
    alarm(distance,2000,40);
  } else {
    delRow(1);
  }
}

float getSonar() {
  unsigned long pingTime;
  float distance;
  digitalWrite(trigPin, HIGH); // make trigPin output high level lasting for 10μs to triger HC_SR04,
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  pingTime = pulseIn(echoPin, HIGH, timeOut); // Wait HC-SR04 returning to the high level and measure out this waitting time
  distance = (float)pingTime * soundVelocity / 2 / 10000; // calculate the distance according to the time
  return distance; // return the distance value
}

void alarm(int dist, int freq, int dur) {
  tone(pinBuzzer,freq,dur);
  buzzerDelay = dist*3;
  Serial.println(buzzerDelay);
  delay(buzzerDelay);
}

void delRow(int row) {
  lcd.setCursor(0,row);
  lcd.print("                ");
}
