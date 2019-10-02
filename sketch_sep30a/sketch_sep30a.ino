#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
//DISPLAY OLED DA COLLEGARE SEMPRE A SCL E SDA
#define OLED_ADDR   0x3C

Adafruit_SSD1306 display(128, 64, &Wire);

//Variables
int counter=0;
int x,y;
int clcd, slcd=0;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();

  delay(1000);
}

void loop() {
  display.clearDisplay();
  x=counter%128;
  y=counter%64;
  
  display.drawPixel(x, y, WHITE);
  display.drawPixel(127-x, y, WHITE);
  display.drawPixel(x, 63-y, WHITE);
  display.drawPixel(127-x, 63-y, WHITE);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(x,30);
  display.print("Hello, world!");

  display.display();

  counter++;
  delay(100);

}
