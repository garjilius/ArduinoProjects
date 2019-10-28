/*
  Sketch 16.1.1
  Display the string on LCD1602

  modified 2016/5/13
  by http://www.freenove.com
*/

#include <LiquidCrystal.h>
#include "DHT.h"
#define DHTPIN 8
#define DHTTYPE DHT11

DHT dht(8, DHT11);;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  dht.begin();
  // Print a message to the LCD
  Serial.begin(9600);
}

void loop() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT");
    lcd.setCursor(13,1);
    lcd.print("(!)");
    delay(5000);
  } else  {
    
    lcd.setCursor(0, 0);
    lcd.print("Humidity:");
    lcd.print(h);
    lcd.print("%");
    lcd.setCursor(0,1);
    lcd.print("Temp.: ");
    lcd.print(t);
    lcd.println("C   "); 

  }

  delay(2000);
}
