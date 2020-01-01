// For the OLED display
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "DHT.h"

#define OLED_ADDR 0x3c// OLED display TWI address
#define DHTPIN 8     // Digital pin connected to the DHT sensor

// reset pin not used on 4-pin OLED module
Adafruit_SSD1306 display(-1);  // -1 = no reset pin
// 128 x 64 pixel display
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define DHTTYPE DHT22     // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);
int i = 0;

void setup() {
  Serial.begin(9600);
  pinMode(7,OUTPUT);
  digitalWrite(7,HIGH);
  display.setTextColor(WHITE);
  // For the OLED display
  // initialize and clear display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();
  dht.begin();
}

void loop() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // For the OLED display
  display.clearDisplay();

  // display a line of text
  display.setCursor(0,0);
  display.setTextSize(1);
  display.print("Temp: ");
  display.print(t);
  display.println(" C");
  display.print("Hum: ");
  display.print(h);
  display.println(" %");

  // update display with all of the above graphics
  display.display();
  delay(1000);

}
