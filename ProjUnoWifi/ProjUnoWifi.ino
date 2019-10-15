#include <DHT.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <BlynkSimpleWiFiNINA.h>
#include <virtuabotixRTC.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header //

#define DHTPIN 2
#define IRPIN 12
#define MOVLED 13
#define SYSLED 11
hd44780_I2Cexp lcd; // declare lcd object: auto locate & config display for hd44780 chip

#define BLYNK_PRINT Serial

//La posizione degli elementi del'array notifiche relativi ai vari eventi
#define EVHUM 0
#define EVTEMP 1
#define EVMOV 2

float humLimit = 75;
float tempLimit = 25;

//Forse mi servirà un nuovo token
char auth[] = "cYc4mGATJA7eiiACUErh33-J6OMEYoKY";
unsigned long lastNotification;
unsigned long currentMillis;
const unsigned long delayNotificationMillis = 60000;
bool notificationAllowed[3] = {true, true, true};
bool systemDisabled = false;
virtuabotixRTC myRTC(7, 6, 5);


WidgetTerminal terminal(V1);


// Uncomment whatever type you're using!
//#define DHTTYPE DHT11     // DHT 11
#define DHTTYPE DHT22   // DHT 22, AM2302, AM2321 <--- Tipo del lab
//#define DHTTYPE DHT21   // DHT 21, AM2301

DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;
char ssid[] = "iChief 6s";
char pass[] = "garjiliusnet27";

BLYNK_CONNECTED() {
  // Request Blynk server to re-send latest values for all pins
  Blynk.syncAll();
}

void sendSensor()
{
  terminal.flush(); //mi assicuro che il terminale non arrivi spezzettato

  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Blynk.virtualWrite(V5, h);
  Blynk.virtualWrite(V6, t);

  //IL SISTEMA VIENE DISABILITATO MA DOPO AVERE COMUNQUE PRESO TEMP E HUM
  if (systemDisabled == 1) {
    digitalWrite(SYSLED, LOW);
    digitalWrite(MOVLED, LOW);
    return; //Se il sistema è disabilitato, non fa nulla
  }
  digitalWrite(SYSLED, HIGH);

  //GESTISCO LE NOTIFICHE DEI SENSORI
  if (h > humLimit) {
    if (notificationAllowed[EVHUM] == true) {
      notificationAllowed[EVHUM] = false;
      String notifica = "L'umidità ha raggiunto valori troppo elevati: ";
      notifica += h;
      notifica += "% alle:";
      notifica += printTime();
      //Blynk.email("Umidità", notifica); //Esempio di email
      Blynk.notify(notifica);
    }
  }
  else {
    notificationAllowed[EVHUM] = true;
  }

  if (t > tempLimit) {
    if (notificationAllowed[EVTEMP] == true) {
      notificationAllowed[EVTEMP] = false;
      String notifica = "La temperatura ha raggiunto valori troppo elevati: ";
      notifica += t;
      notifica += " C alle";
      notifica += printTime();

      //Blynk.email("Temperatura", notifica); //Esempio di email
      Blynk.notify(notifica);
    }
  }
  else {
    notificationAllowed[EVTEMP] = true;
  }

  if (digitalRead(IRPIN) == HIGH) {
    digitalWrite(MOVLED, HIGH);
    terminal.println("Movimento Rilevato");
    if (notificationAllowed[EVMOV] == true) {
      notificationAllowed[EVMOV] = false;
      String notifica = "Rilevato un movimento! Alle ";
      notifica += printTime();
      //Blynk.email("Temperatura", notifica); //Esempio di email
      Blynk.notify(notifica);

    }
  }
  else {
    digitalWrite(MOVLED, LOW);
    timer.setInterval(60000L, enableMovementNotification);
  }
}

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  lcd.begin(20, 4);
  lcd.print("Initializing...");
  // Set the current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year

  myRTC.setDS1302Time(00, 20, 12, 1, 14, 10, 2019);

  lastNotification = millis();
  syncWidgets();

  pinMode(MOVLED, OUTPUT);
  pinMode(SYSLED, OUTPUT);
  dht.begin();

  // Setup a function to be called every second
  timer.setInterval(1000L, sendSensor);
  //Ogni secondo stampa a terminale quali notifiche sono consentite e quali no
  timer.setInterval(5000L, debugSystem);
  timer.setInterval(1000L, syncWidgets);
  timer.setInterval(5000L, printWifiData);
  timer.setInterval(5000L, printCurrentNet);
  timer.setInterval(1000L, handleDisplay);
}


void loop()
{
  Blynk.run();
  timer.run();
  myRTC.updateTime();
}

//Per il movimento è necessario usare timer per resettare le notifiche
void enableMovementNotification() {
  notificationAllowed[EVMOV] = true;
}

void debugSystem() {
  terminal.println("");
  terminal.print("HumLimit:" );
  terminal.println(humLimit);
  terminal.print("TempLimit: ");
  terminal.println(tempLimit);
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
  if (systemDisabled == 1) {
    terminal.println("System Disabled");
  }
  else {
    terminal.println("Sistem Enabled");
  }
}

void syncWidgets() {
  Blynk.virtualWrite(V3, tempLimit);
  Blynk.virtualWrite(V2, humLimit);
  Blynk.virtualWrite(V0, systemDisabled);
}

BLYNK_WRITE(V0)  {
  systemDisabled = param.asInt();
}

BLYNK_WRITE(V3)  {
  tempLimit = param.asFloat();
  terminal.print("Temp Limit: ");
  terminal.println(tempLimit);
}

BLYNK_WRITE(V2)  {
  humLimit = param.asFloat();
  terminal.print("Hum Limit: ");
  terminal.println(humLimit);
}

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

String printTime() {
  String orario = "";
  orario += myRTC.hours;
  orario += ":";
  orario += myRTC.minutes;
  orario += ":";
  orario += myRTC.seconds;
  return orario;
}

void handleDisplay() {
    lcd.clear();
    String time = "Time ";
    time += printTime();
    lcd.setCursor(0,0);
    lcd.print(time);
}
