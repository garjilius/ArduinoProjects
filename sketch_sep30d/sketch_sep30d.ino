#include <LiquidCrystal_I2C.h>
#include <Wire.h>
//-----Hardware Addressing-----
//If your display does not work, comment the line below and uncomment //the other address-line
//LiquidCrystal_I2C lcd(0x27,20,4);
LiquidCrystal_I2C lcd(0x3F,20,4);
void setup() {
  lcd.init();
}
void loop() {
  lcd.backlight();
  //Print message
lcd.setCursor(0,0); lcd.print(" joy-IT"); lcd.setCursor(0,1); lcd.print(" "); lcd.setCursor(0,2); lcd.print(" I2C Serial"); lcd.setCursor(0,3); lcd.print(" LCD");
}
