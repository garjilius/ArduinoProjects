#include <virtuabotixRTC.h>

virtuabotixRTC myRTC(7, 6, 5);

void setup() {
  Serial.begin(9600);
  // Set the current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  myRTC.setDS1302Time(00, 55, 11, 1, 30, 9, 2019);
}

void loop() {
// This allows for the update of variables for time or accessing the individual elements.
  myRTC.updateTime();
  
// Start printing elements as individuals
  Serial.print("Clock: ");
  print2chars(myRTC.dayofmonth);
  Serial.print("/");
  print2chars(myRTC.month);
  Serial.print("/");
  print2chars(myRTC.year);
  Serial.print("  ");
  print2chars(myRTC.hours);
  Serial.print(":");
  print2chars(myRTC.minutes);
  Serial.print(":");
  print2chars(myRTC.seconds);    
  Serial.println("");
  
// Delay so the program doesn't print non-stop
  delay( 5000); // 
}

void print2chars(int n) {
    if (n<10) {
      Serial.print("0");
    }
    Serial.print(n);
}
