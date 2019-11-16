#include <LiquidCrystal.h>

LiquidCrystal lcd(11, 10, 5, 4, 3, 2);

/*
   Usa il Bluetooth per leggere stringhe via seriale e stamparle sul display
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
    Serial.print(s);
    //delay(5000);
    //printStringChar(s);
  }
}

void lcdPrint(String s) {
  if (s.length() > 16) { //Va a capo se la stringa è più lunga della riga del display, saltando lo spazio a inizio seconda riga, se c'è
    String s1 = s.substring(0, 16);
    lcd.print(s1);
    Serial.println(s[16]);
    lcd.setCursor(0, 1);
    if (s[16] != ' ') {
      String s2 = s.substring(16);
      lcd.print(s2);
    } else {
      Serial.println("Salto Spazio");
      String s2 = s.substring(17);
      lcd.print(s2);
    }
  } else {
    lcd.print(s);
  }
}

/*
   funzione sperimentale che doveva servire a stampare con autoscroll,
   non funziona bene

  void printStringChar(String s) {
  lcd.clear();
  lcd.setCursor(0,0);
  for (int i = 0; i < s.length(); i++) {
    lcd.autoscroll();
    Serial.print(s[i]);
    lcd.print(s[i]);
    delay(500);
  }
  //lcd.noAutoscroll();
  }
*/
