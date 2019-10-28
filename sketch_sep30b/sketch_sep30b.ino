// For the LCD display
//#include <LiquidCrystal_I2C.h>
#include <hd44780.h>
//LiquidCrystal_I2C lcd(0x3F,20,4);  // set the LCD address for a 16 chars and 2 line display


//Variables
int counter=0;
int x,y;
int clcd, slcd=0;

void setup() {
  // For the LCD display
  lcd.init();                      
  lcd.backlight();
  lcd.print("Test Display I2C");
  lcd.setCursor(0,1);
  lcd.print("Hello!");
  
}

void loop() {
  // For the LCD display

  if (clcd==5) {
    clcd=0;
    lcd.clear();
    x = slcd % 16;
    lcd.setCursor(x,0);
    lcd.print("scroll right");
    lcd.setCursor(x,1);
    x = 15-(slcd % 16);
    lcd.setCursor(x,1);
    lcd.print("scroll left");
    slcd++;
  }

}
