
#include <SoftwareSerial.h>
#include <BlynkSimpleStream.h>
#include <DHT.h>

#define DHTPIN 2
#define BLYNK_PRINT SwSerial
#define EVHUM 0
#define EVTEMP 1
#define EVMOV 2
#define HUMLIMIT 75
#define TEMPLIMIT 25
#define IRPIN 12

char auth[] = "KjRZz0ewLqAP38p2kX6_TqnLXuWoYumK";
unsigned long lastNotification;
unsigned long currentMillis;
const unsigned long delayNotificationMillis = 60000;
bool notificationAllowed[5] = {true, true, true, true, true};

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
  terminal.flush(); //mi assicuro che il terminale non arrivi spezzettato
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

  //GESTISCO LE NOTIFICHE DEI SENSORI
  if (h > HUMLIMIT) {
    if (notificationAllowed[EVHUM] == true) {
      notificationAllowed[EVHUM] = false;
      String notifica = "L'umidità ha raggiunto valori troppo elevati: ";
      notifica += h;
      notifica += "%";
      //Blynk.email("Umidità", notifica); //Esempio di email
      Blynk.notify(notifica);
    }
  }
  else {
    notificationAllowed[EVHUM] = true;
  }

  if (t > TEMPLIMIT) {
    if (notificationAllowed[EVTEMP] == true) {
      notificationAllowed[EVTEMP] = false;
      String notifica = "La temperatura ha raggiunto valori troppo elevati: ";
      notifica += t;
      notifica += " C";
      //Blynk.email("Temperatura", notifica); //Esempio di email
      Blynk.notify(notifica);
    }
  }
  else {
    notificationAllowed[EVTEMP] = true;
  }

  if (digitalRead(IRPIN) == HIGH) {
    terminal.println("Movimento Rilevato");
    if (notificationAllowed[EVMOV] == true) {
      notificationAllowed[EVMOV] = false;
      String notifica = "Rilevato un movimento!";
      //Blynk.email("Temperatura", notifica); //Esempio di email
      Blynk.notify(notifica);
      digitalWrite(13, HIGH);
    }
  }
  else {
    digitalWrite(13, LOW);
    timer.setInterval(60000L, enableMovementNotification);
  }
}

void setup()
{
  // Debug console
  SwSerial.begin(9600);
  lastNotification = millis();

  // Blynk will work through Serial
  // Do not read or write this serial manually in your sketch
  Serial.begin(9600);
  Blynk.begin(Serial, auth);
  pinMode(13, OUTPUT);

  dht.begin();

  // Setup a function to be called every second
  timer.setInterval(1000L, sendSensor);
  //Ogni secondo stampa a terminale quali notifiche sono consentite e quali no
  timer.setInterval(1000L, debugNotifications);
}

void loop()
{
  Blynk.run();
  timer.run();
}

//Per il movimento è necessario usare timer per resettare le notifiche
void enableMovementNotification() {
  notificationAllowed[EVMOV] = true;
}

void debugNotifications() {
  terminal.println("");
  for (int i = 0; i < 3; i++) {
    if (notificationAllowed[i]) {
      terminal.print(i);
      terminal.print(":allowed at ms ");
    }
    else {
      terminal.print(i);
      terminal.print(":NOT ALLOWED at ms ");
    }
    terminal.println(millis());
    terminal.flush();
  }
}
