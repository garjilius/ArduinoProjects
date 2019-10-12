
#include <SoftwareSerial.h>
#include <BlynkSimpleStream.h>
#include <DHT.h>

#define DHTPIN 2
#define BLYNK_PRINT SwSerial
#define EVHUM 0
#define EVTEMP 1
#define HUMLIMIT 75
#define TEMPLIMIT 25

char auth[] = "KjRZz0ewLqAP38p2kX6_TqnLXuWoYumK";
unsigned long lastNotification;
unsigned long currentMillis;
const unsigned long delayNotificationMillis = 60000;
bool notificationAllowed[5] = {true,true,true,true,true}; //uso g

SoftwareSerial SwSerial(10, 11); // RX, TX
WidgetTerminal terminal(V1);


// Uncomment whatever type you're using!
#define DHTTYPE DHT11     // DHT 11
//#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321
//#define DHTTYPE DHT21   // DHT 21, AM2301

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

void sendSensor()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    SwSerial.println("Failed to read from DHT sensor!");
    return;
  }
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V5, h);
  Blynk.virtualWrite(V6, t);

  if (h > HUMLIMIT) {
    if (notificationAllowed[EVHUM] == true) {
      notificationAllowed[EVHUM] = false;
      String notifica = "L'umidità ha raggiunto valori troppo elevati: ";
      notifica += h;
      notifica += "%";
      //Blynk.email("Umidità", notifica); //Esempio di email
      Blynk.notify(notifica);
      terminal.println("Notification supposedly sent");
    }
  } 
  else {
    notificationAllowed[EVHUM] = true;
  }
   
}

void setup()
{
  // Debug console
  SwSerial.begin(9600);
  terminal.clear();
  lastNotification = millis();

  // Blynk will work through Serial
  // Do not read or write this serial manually in your sketch
  Serial.begin(9600);
  Blynk.begin(Serial, auth);

  dht.begin();

  // Setup a function to be called every second
  timer.setInterval(1000L, sendSensor);
}

void loop()
{
  Blynk.run();
  timer.run();
}

/*
bool notificationAllowed() {
  currentMillis = millis();
if (lastNotification - currentMillis > delayNotificationMillis ) {
    
    terminal.print("Allowed - ");
    terminal.print("Difference: ");
    terminal.println((currentMillis-lastNotification)/1000);
    
    lastNotification = currentMillis;
    return true;
  }
  else {
    terminal.print("NOT Allowed - ");
    terminal.print("Difference: ");
    terminal.println((currentMillis-lastNotification)/1000);  
    return false;
  }
}
*/
