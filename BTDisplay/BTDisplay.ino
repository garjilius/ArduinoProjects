#include <LiquidCrystal.h>

LiquidCrystal lcd(11, 10, 5, 4, 3, 2);

/*
 * Usa il Bluetooth per leggere stringhe via seriale e stamparle sul display
 */

void setup() {
  Serial.begin(9600);
  Serial.println("Starting ...");
  lcd.begin(16, 2);
  lcd.print("AVVIATO");
  delay(1000);
}


void loop() {
  if (Serial.available()) {
    lcd.clear();
    String s = Serial.readString();
    lcdPrint(s);
    //Serial.println(s);
  }
}

void lcdPrint(String s) {
  if (s.length() > 16) { //Va a capo se la stringa è più lunga della riga del display
      String s1 = s.substring(0, 16);
      String s2 = s.substring(16);
      lcd.print(s1);
      lcd.setCursor(0, 1);
      lcd.print(s2);
    } else {
      lcd.print(s);
    }
}
